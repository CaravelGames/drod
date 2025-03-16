// $Id: TileImageCalcs.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "TileImageCalcs.h"
#include "RoomWidget.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/DbRooms.h"
#include <FrontEndLib/BitmapManager.h>
#include <BackEndLib/Assert.h>

//*****************************************************************************
//Prototypes.
UINT  CalcTileImageForFourNeighborCC(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, const UINT wTileNo);
UINT  CalcTileImageForFuse(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForHiddenWall(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForBrokenWall(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForObstacle(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForTarstuff(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForWall(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForPlatform(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, const UINT wTileNo);
UINT  CalcTileImageForBriar(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, const UINT wTileNo);
UINT  CalcTileImageForMist(const CDbRoom* pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForStairsUp(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);

//Definitions
const UINT MonsterTileImageArray[MONSTER_TYPES][ORIENTATION_COUNT] = {
	//nw                    n                       ne
	//w                        none                 e
	//sw                    s                       se

	//M_ROACH
	{
	TI_ROACH_NW,         TI_ROACH_N,       TI_ROACH_NE,
	TI_ROACH_W,          TI_ROACH_S,       TI_ROACH_E,
	TI_ROACH_SW,         TI_ROACH_S,       TI_ROACH_SE
	},

	//M_QROACH
	{
	TI_QROACH_NW,        TI_QROACH_N,      TI_QROACH_NE,
	TI_QROACH_W,         TI_QROACH_S,      TI_QROACH_E,
	TI_QROACH_SW,        TI_QROACH_S,      TI_QROACH_SE
	},

	//M_REGG (Egg deposited at 8 o'clock, born at midnight.)
	{
	TI_REGG_3,           TI_REGG_4,        TI_REGG_1,
	TI_REGG_2,           TI_REGG_4,        TI_REGG_2,
	TI_REGG_1,           TI_REGG_4,        TI_REGG_3
	},

	//M_GOBLIN
	{
	TI_GOBLIN_NW,        TI_GOBLIN_N,      TI_GOBLIN_NE,
	TI_GOBLIN_W,         TI_GOBLIN_S,      TI_GOBLIN_E,
	TI_GOBLIN_SW,        TI_GOBLIN_S,      TI_GOBLIN_SE
	},

	//M_NEATHER
	{
	TI_HALPH_NW,            TI_HALPH_N,          TI_HALPH_NE,
	TI_HALPH_W,             TI_HALPH_S,          TI_HALPH_E,
	TI_HALPH_SW,            TI_HALPH_S,          TI_HALPH_SE
	},

	//M_WWING
	{
	TI_WW_NW,               TI_WW_N,          TI_WW_NE,
	TI_WW_W,                TI_WW_S,          TI_WW_E,
	TI_WW_SW,               TI_WW_S,          TI_WW_SE
	},

	//M_EYE
	{
	TI_EYE_NW,           TI_EYE_N,            TI_EYE_NE,
	TI_EYE_W,            TI_EYE_S,            TI_EYE_E,
	TI_EYE_SW,           TI_EYE_S,            TI_EYE_SE
	},

	//M_SERPENT
	{
	DONT_USE,               TI_SNK_N,            DONT_USE,
	TI_SNK_W,               DONT_USE,            TI_SNK_E,
	DONT_USE,               TI_SNK_S,            DONT_USE
	},

	//M_TARMOTHER
	{
	TI_TAREYE_WO,        TI_TAREYE_WO,           TI_TAREYE_WO,
	TI_TAREYE_WC,        TI_TAREYE_WO,           TI_TAREYE_WO,
	TI_TAREYE_EC,        TI_TAREYE_EO,           TI_TAREYE_WO
	},

	//M_TARBABY
	{
	TI_TARBABY_NW,         TI_TARBABY_N,       TI_TARBABY_NE,
	TI_TARBABY_W,          TI_TARBABY_S,       TI_TARBABY_E,
	TI_TARBABY_SW,         TI_TARBABY_S,       TI_TARBABY_SE
	},

	//M_BRAIN
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_BRAIN,            DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	},

	//M_MIMIC
	{
	TI_MIMIC_NW,         TI_MIMIC_N,       TI_MIMIC_NE,
	TI_MIMIC_W,          TI_MIMIC_S,       TI_MIMIC_E,
	TI_MIMIC_SW,         TI_MIMIC_S,       TI_MIMIC_SE
	},

	//M_SPIDER
	{
	TI_SPIDER_NW,         TI_SPIDER_N,			   TI_SPIDER_NE,
	TI_SPIDER_W,          TI_TEMPTY,			   TI_SPIDER_E,
	TI_SPIDER_SW,         TI_SPIDER_S,			   TI_SPIDER_SE
	},

	//M_SERPENTG
	{
	DONT_USE,               TI_SNK_G_N,          DONT_USE,
	TI_SNK_G_W,             DONT_USE,            TI_SNK_G_E,
	DONT_USE,               TI_SNK_G_S,          DONT_USE
	},

	//M_SERPENTB
	{
	DONT_USE,               TI_SNK_B_N,          DONT_USE,
	TI_SNK_B_W,             DONT_USE,            TI_SNK_B_E,
	DONT_USE,               TI_SNK_B_S,          DONT_USE
	},

	//M_ROCKGOLEM
	{
	TI_ROCK_NW,             TI_ROCK_N,           TI_ROCK_NE,
	TI_ROCK_W,              TI_ROCK_BRKN,        TI_ROCK_E,
	TI_ROCK_SW,             TI_ROCK_S,           TI_ROCK_SE
	},

	//M_WATERSKIPPER
	{
	TI_WATERSKIPPER_NW,     TI_WATERSKIPPER_N,   TI_WATERSKIPPER_NE,
	TI_WATERSKIPPER_W,      TI_WATERSKIPPER_S,   TI_WATERSKIPPER_E,
	TI_WATERSKIPPER_SW,     TI_WATERSKIPPER_S,   TI_WATERSKIPPER_SE
	},

	//M_SKIPPERNEST
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_SKIPPERNEST,      DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	},

	//M_AUMTLICH
	{
	TI_AUMTLICH_NW,         TI_AUMTLICH_N,       TI_AUMTLICH_NE,
	TI_AUMTLICH_W,          TI_AUMTLICH_S,       TI_AUMTLICH_E,
	TI_AUMTLICH_SW,         TI_AUMTLICH_S,       TI_AUMTLICH_SE
	},

	//M_CLONE
	{
	TI_SMAN_YNW,            TI_SMAN_YN,          TI_SMAN_YNE,
	TI_SMAN_YW,             TI_SMAN_YS,          TI_SMAN_YE,
	TI_SMAN_YSW,            TI_SMAN_YS,          TI_SMAN_YSE
	},

	//M_DECOY
	{
	TI_SMAN_IYNW,           TI_SMAN_IYN,         TI_SMAN_IYNE,
	TI_SMAN_IYW,            TI_SMAN_IYS,         TI_SMAN_IYE,
	TI_SMAN_IYSW,           TI_SMAN_IYS,         TI_SMAN_IYSE
	},

	//M_WUBBA
	{
	TI_WUBBA,               TI_WUBBA,            TI_WUBBA,
	TI_WUBBA,               TI_WUBBA,            TI_WUBBA,
	TI_WUBBA,               TI_WUBBA,            TI_WUBBA
	},

	//M_SEEP
	{
	TI_SEEP_NW,             TI_SEEP_N,           TI_SEEP_NE,
	TI_SEEP_W,              TI_SEEP_S,           TI_SEEP_E,
	TI_SEEP_SW,             TI_SEEP_S,           TI_SEEP_SE
	},

	//M_PIRATE
	{
	TI_PIRATE_NW,         TI_PIRATE_N,         TI_PIRATE_NE,
	TI_PIRATE_W,          TI_PIRATE_S,         TI_PIRATE_E,
	TI_PIRATE_SW,         TI_PIRATE_S,         TI_PIRATE_SE
	},

	//M_HALPH
	{
	TI_HALPH_NW,            TI_HALPH_N,          TI_HALPH_NE,
	TI_HALPH_W,             TI_HALPH_S,          TI_HALPH_E,
	TI_HALPH_SW,            TI_HALPH_S,          TI_HALPH_SE
	},

	//M_SLAYER
	{
	TI_SLAYER_NW,           TI_SLAYER_N,         TI_SLAYER_NE,
	TI_SLAYER_W,            TI_SLAYER_S,         TI_SLAYER_E,
	TI_SLAYER_SW,           TI_SLAYER_S,         TI_SLAYER_SE
	},

	//M_FEGUNDO
	{
	TI_FEGUNDO_NW,            TI_FEGUNDO_N,          TI_FEGUNDO_NE,
	TI_FEGUNDO_W,             TI_FEGUNDO_S,          TI_FEGUNDO_E,
	TI_FEGUNDO_SW,            TI_FEGUNDO_S,          TI_FEGUNDO_SE
	},

	//M_FEGUNDOASHES
	{
	DONT_USE,          DONT_USE,        DONT_USE,
	DONT_USE,          DONT_USE,        DONT_USE,
	DONT_USE,          DONT_USE,        DONT_USE
	},

	//M_GUARD
	{
	TI_GUARD_NW,            TI_GUARD_N,          TI_GUARD_NE,
	TI_GUARD_W,             TI_GUARD_S,          TI_GUARD_E,
	TI_GUARD_SW,            TI_GUARD_S,          TI_GUARD_SE
	},

	//M_CHARACTER
	{
	TI_NEGOTIATOR_NW,            TI_NEGOTIATOR_N,          TI_NEGOTIATOR_NE,
	TI_NEGOTIATOR_W,             TI_NEGOTIATOR_S,          TI_NEGOTIATOR_E,
	TI_NEGOTIATOR_SW,            TI_NEGOTIATOR_S,          TI_NEGOTIATOR_SE
	},

	//M_MUDMOTHER
	{
	TI_MUDEYE_WO,        TI_MUDEYE_WO,           TI_MUDEYE_WO,
	TI_MUDEYE_WC,        TI_MUDEYE_WO,           TI_MUDEYE_WO,
	TI_MUDEYE_EC,        TI_MUDEYE_EO,           TI_MUDEYE_WO
	},

	//M_MUDBABY
	{
	TI_MUDBABY_NW,          TI_MUDBABY_N,       TI_MUDBABY_NE,
	TI_MUDBABY_W,           TI_MUDBABY_S,       TI_MUDBABY_E,
	TI_MUDBABY_SW,          TI_MUDBABY_S,       TI_MUDBABY_SE
	},

	//M_GELMOTHER
	{
	TI_GELEYE_WO,        TI_GELEYE_WO,           TI_GELEYE_WO,
	TI_GELEYE_WC,        TI_GELEYE_WO,           TI_GELEYE_WO,
	TI_GELEYE_EC,        TI_GELEYE_EO,           TI_GELEYE_WO
	},

	//M_GELBABY
	{
	TI_GELBABY_NW,          TI_GELBABY_N,       TI_GELBABY_NE,
	TI_GELBABY_W,           TI_GELBABY_S,       TI_GELBABY_E,
	TI_GELBABY_SW,          TI_GELBABY_S,       TI_GELBABY_SE
	},

	//M_CITIZEN
	{
	TI_CITIZEN_NW,            TI_CITIZEN_N,      TI_CITIZEN_NE,
	TI_CITIZEN_W,             TI_CITIZEN_S,      TI_CITIZEN_E,
	TI_CITIZEN_SW,            TI_CITIZEN_S,      TI_CITIZEN_SE
	},

	//M_ROCKGIANT
	{
	TI_ROCKGIANT_NW,            TI_ROCKGIANT_N,      TI_ROCKGIANT_NE,
	TI_ROCKGIANT_W,             TI_ROCKGIANT_S,      TI_ROCKGIANT_E,
	TI_ROCKGIANT_SW,            TI_ROCKGIANT_S,      TI_ROCKGIANT_SE
	},

	//M_MADEYE
	{
	TI_EYE_WNW,               TI_EYE_WN,           TI_EYE_WNE,
	TI_EYE_WW,                TI_EYE_WS,           TI_EYE_WE,
	TI_EYE_WSW,               TI_EYE_WS,           TI_EYE_WSE
	},

	//M_GOBLINKING
	{
	TI_GOBLINKING_NW,             TI_GOBLINKING_N,        TI_GOBLINKING_NE,
	TI_GOBLINKING_W,              TI_GOBLINKING_S,        TI_GOBLINKING_E,
	TI_GOBLINKING_SW,             TI_GOBLINKING_S,        TI_GOBLINKING_SE
	},

	//M_CONSTRUCT
	{
	TI_CONSTRUCT_NW,       TI_CONSTRUCT_N,      TI_CONSTRUCT_NE,
	TI_CONSTRUCT_W,        TI_CONSTRUCT_S,      TI_CONSTRUCT_E,
	TI_CONSTRUCT_SW,       TI_CONSTRUCT_S,      TI_CONSTRUCT_SE
	},

	//M_FLUFFBABY
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_FLUFFBABY,            DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	}
};

const UINT AnimatedMonsterTileImageArray[MONSTER_TYPES][ORIENTATION_COUNT] = {
	//nw                    n                       ne
	//w                        none                 e
	//sw                    s                       se

	//M_ROACH
	{
	TI_ROACH_ANW,        TI_ROACH_AN,      TI_ROACH_ANE,
	TI_ROACH_AW,         TI_ROACH_AS,      TI_ROACH_AE,
	TI_ROACH_ASW,        TI_ROACH_AS,      TI_ROACH_ASE
	},

	//M_QROACH
	{
	TI_QROACH_ANW,    TI_QROACH_AN,     TI_QROACH_ANE,
	TI_QROACH_AW,     TI_QROACH_AS,     TI_QROACH_AE,
	TI_QROACH_ASW,    TI_QROACH_AS,     TI_QROACH_ASE
	},

	//M_REGG (Egg deposited at 8 o'clock, born at midnight.)
	{
	TI_REGG_A3,          TI_REGG_A4,       TI_REGG_A1,
	TI_REGG_A2,          TI_REGG_A4,       TI_REGG_A2,
	TI_REGG_A1,          TI_REGG_A4,       TI_REGG_A3
	},

	//M_GOBLIN
	{
	TI_GOBLIN_ANW,    TI_GOBLIN_AN,     TI_GOBLIN_ANE,
	TI_GOBLIN_AW,     TI_GOBLIN_AS,     TI_GOBLIN_AE,
	TI_GOBLIN_ASW,    TI_GOBLIN_AS,     TI_GOBLIN_ASE
	},

	//M_NEATHER
	{
	TI_HALPH_NW,            TI_HALPH_N,          TI_HALPH_NE,
	TI_HALPH_W,             TI_HALPH_S,          TI_HALPH_E,
	TI_HALPH_SW,            TI_HALPH_S,          TI_HALPH_SE
	},

	//M_WWING
	{
	TI_WW_ANW,           TI_WW_AN,            TI_WW_ANE,
	TI_WW_AW,            TI_WW_AS,            TI_WW_AE,
	TI_WW_ASW,           TI_WW_AS,            TI_WW_ASE
	},

	//M_EYE
	{
	TI_EYE_ANW,          TI_EYE_AN,        TI_EYE_ANE,
	TI_EYE_AW,           TI_EYE_AS,         TI_EYE_AE,
	TI_EYE_ASW,          TI_EYE_AS,        TI_EYE_ASE
	},

	//M_SERPENT
	{
	DONT_USE,               TI_SNK_AN,        DONT_USE,
	TI_SNK_AW,              DONT_USE,         TI_SNK_AE,
	DONT_USE,               TI_SNK_AS,        DONT_USE
	},

	//M_TARMOTHER
	{
	TI_TAREYE_WO,        TI_TAREYE_WO,           TI_TAREYE_WO,
	TI_TAREYE_WC,        TI_TAREYE_WO,           TI_TAREYE_WO,
	TI_TAREYE_EC,        TI_TAREYE_EO,           TI_TAREYE_WO
	},

	//M_TARBABY
	{
	TI_TARBABY_ANW,        TI_TARBABY_AN,     TI_TARBABY_ANE,
	TI_TARBABY_AW,         TI_TARBABY_AS,     TI_TARBABY_AE,
	TI_TARBABY_ASW,        TI_TARBABY_AS,     TI_TARBABY_ASE
	},

	//M_BRAIN
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_BRAIN_A,          DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	},

	//M_MIMIC
	{
	TI_MIMIC_NW,         TI_MIMIC_N,       TI_MIMIC_NE,
	TI_MIMIC_W,          TI_MIMIC_S,       TI_MIMIC_E,
	TI_MIMIC_SW,         TI_MIMIC_S,       TI_MIMIC_SE
	},

	//M_SPIDER
	{
	TI_SPIDER_ANW,        TI_SPIDER_AN,			   TI_SPIDER_ANE,
	TI_SPIDER_AW,         TI_SPIDER_AS,			   TI_SPIDER_AE,
	TI_SPIDER_ASW,        TI_SPIDER_AS,			   TI_SPIDER_ASE
	},

	//M_SERPENTG
	{
	DONT_USE,               TI_SNK_G_AN,         DONT_USE,
	TI_SNK_G_AW,            DONT_USE,            TI_SNK_G_AE,
	DONT_USE,               TI_SNK_G_AS,         DONT_USE
	},

	//M_SERPENTB
	{
	DONT_USE,               TI_SNK_B_AN,         DONT_USE,
	TI_SNK_B_AW,            DONT_USE,            TI_SNK_B_AE,
	DONT_USE,               TI_SNK_B_AS,         DONT_USE
	},

	//M_ROCKGOLEM
	{
	TI_ROCK_ANW,            TI_ROCK_AN,          TI_ROCK_ANE,
	TI_ROCK_AW,             TI_ROCK_ABRKN,       TI_ROCK_AE,
	TI_ROCK_ASW,            TI_ROCK_AS,          TI_ROCK_ASE
	},

	//M_WATERSKIPPER
	{
	TI_WATERSKIPPER_ANW,    TI_WATERSKIPPER_AN,  TI_WATERSKIPPER_ANE,
	TI_WATERSKIPPER_AW,     DONT_USE,            TI_WATERSKIPPER_AE,
	TI_WATERSKIPPER_ASW,    TI_WATERSKIPPER_AS,  TI_WATERSKIPPER_ASE
	},

	//M_SKIPPERNEST
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_SKIPPERNEST,      DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	},

	//M_AUMTLICH
	{
	TI_AUMTLICH_ANW,        TI_AUMTLICH_AN,      TI_AUMTLICH_ANE,
	TI_AUMTLICH_AW,         TI_AUMTLICH_AS,      TI_AUMTLICH_AE,
	TI_AUMTLICH_ASW,        TI_AUMTLICH_AS,      TI_AUMTLICH_ASE
	},

	//M_CLONE
	{
	TI_SMAN_YNW,            TI_SMAN_YN,          TI_SMAN_YNE,
	TI_SMAN_YW,             TI_SMAN_YS,          TI_SMAN_YE,
	TI_SMAN_YSW,            TI_SMAN_YS,          TI_SMAN_YSE
	},

	//M_DECOY
	{
	TI_SMAN_IYNW,           TI_SMAN_IYN,         TI_SMAN_IYNE,
	TI_SMAN_IYW,            TI_SMAN_IYS,         TI_SMAN_IYE,
	TI_SMAN_IYSW,           TI_SMAN_IYS,         TI_SMAN_IYSE
	},

	//M_WUBBA
	{
	TI_WUBBA_A,             TI_WUBBA_A,          TI_WUBBA_A,
	TI_WUBBA_A,             TI_WUBBA_A,          TI_WUBBA_A,
	TI_WUBBA_A,             TI_WUBBA_A,          TI_WUBBA_A
	},

	//M_SEEP
	{
	TI_SEEP_ANW,            TI_SEEP_AN,          TI_SEEP_ANE,
	TI_SEEP_AW,             TI_SEEP_AS,          TI_SEEP_AE,
	TI_SEEP_ASW,            TI_SEEP_AS,          TI_SEEP_ASE
	},

	//M_PIRATE
	{
	TI_PIRATE_NW,         TI_PIRATE_N,         TI_PIRATE_NE,
	TI_PIRATE_W,          TI_PIRATE_S,         TI_PIRATE_E,
	TI_PIRATE_SW,         TI_PIRATE_S,         TI_PIRATE_SE
	},

	//M_HALPH
	{
	TI_HALPH_NW,           TI_HALPH_N,         TI_HALPH_NE,
	TI_HALPH_W,            TI_HALPH_S,         TI_HALPH_E,
	TI_HALPH_SW,           TI_HALPH_S,         TI_HALPH_SE
	},

	//M_SLAYER
	{
	TI_SLAYER_NW,          TI_SLAYER_N,        TI_SLAYER_NE,
	TI_SLAYER_W,           TI_SLAYER_S,        TI_SLAYER_E,
	TI_SLAYER_SW,          TI_SLAYER_S,        TI_SLAYER_SE
	},

	//M_FEGUNDO
	{
	TI_FEGUNDO_ANW,           TI_FEGUNDO_AN,         TI_FEGUNDO_ANE,
	TI_FEGUNDO_AW,            TI_FEGUNDO_AS,         TI_FEGUNDO_AE,
	TI_FEGUNDO_ASW,           TI_FEGUNDO_AS,         TI_FEGUNDO_ASE
	},

	//M_FEGUNDOASHES
	{
	DONT_USE,          DONT_USE,        DONT_USE,
	DONT_USE,          DONT_USE,        DONT_USE,
	DONT_USE,          DONT_USE,        DONT_USE
	},

	//M_GUARD
	{
	TI_GUARD_NW,           TI_GUARD_N,         TI_GUARD_NE,
	TI_GUARD_W,            TI_GUARD_S,         TI_GUARD_E,
	TI_GUARD_SW,           TI_GUARD_S,         TI_GUARD_SE
	},

	//M_CHARACTER
	{
	TI_NEGOTIATOR_NW,         TI_NEGOTIATOR_N,       TI_NEGOTIATOR_NE,
	TI_NEGOTIATOR_W,          TI_NEGOTIATOR_S,       TI_NEGOTIATOR_E,
	TI_NEGOTIATOR_SW,         TI_NEGOTIATOR_S,       TI_NEGOTIATOR_SE
	},

	//M_MUDMOTHER
	{
	TI_MUDEYE_WO,        TI_MUDEYE_WO,           TI_MUDEYE_WO,
	TI_MUDEYE_WC,        TI_MUDEYE_WO,           TI_MUDEYE_WO,
	TI_MUDEYE_EC,        TI_MUDEYE_EO,           TI_MUDEYE_WO
	},

	//M_MUDBABY
	{
	TI_MUDBABY_ANW,        TI_MUDBABY_AN,     TI_MUDBABY_ANE,
	TI_MUDBABY_AW,         TI_MUDBABY_AS,     TI_MUDBABY_AE,
	TI_MUDBABY_ASW,        TI_MUDBABY_AS,     TI_MUDBABY_ASE
	},

	//M_GELMOTHER
	{
	TI_GELEYE_WO,        TI_GELEYE_WO,           TI_GELEYE_WO,
	TI_GELEYE_WC,        TI_GELEYE_WO,           TI_GELEYE_WO,
	TI_GELEYE_EC,        TI_GELEYE_EO,           TI_GELEYE_WO
	},

	//M_GELBABY
	{
	TI_GELBABY_ANW,          TI_GELBABY_AN,       TI_GELBABY_ANE,
	TI_GELBABY_AW,           TI_GELBABY_AS,       TI_GELBABY_AE,
	TI_GELBABY_ASW,          TI_GELBABY_AS,       TI_GELBABY_ASE
	},

	//M_CITIZEN
	{
	TI_CITIZEN_NW,            TI_CITIZEN_N,      TI_CITIZEN_NE,
	TI_CITIZEN_W,             TI_CITIZEN_S,      TI_CITIZEN_E,
	TI_CITIZEN_SW,            TI_CITIZEN_S,      TI_CITIZEN_SE
	},

	//M_ROCKGIANT
	{
	TI_ROCKGIANTA_NW,            TI_ROCKGIANTA_N,      TI_ROCKGIANTA_NE,
	TI_ROCKGIANTA_W,             TI_ROCKGIANTA_S,      TI_ROCKGIANTA_E,
	TI_ROCKGIANTA_SW,            TI_ROCKGIANTA_S,      TI_ROCKGIANTA_SE
	},

	//M_MADEYE
	{
	TI_EYE_WNW,               TI_EYE_WN,           TI_EYE_WNE,
	TI_EYE_WW,                TI_EYE_WS,           TI_EYE_WE,
	TI_EYE_WSW,               TI_EYE_WS,           TI_EYE_WSE
	},

	//M_GOBLINKING
	{
	TI_GOBLINKING_NW,             TI_GOBLINKING_N,        TI_GOBLINKING_NE,
	TI_GOBLINKING_W,              TI_GOBLINKING_S,        TI_GOBLINKING_E,
	TI_GOBLINKING_SW,             TI_GOBLINKING_S,        TI_GOBLINKING_SE
	},

	//M_CONSTRUCT
	{
	TI_CONSTRUCT_ANW,       TI_CONSTRUCT_AN,      TI_CONSTRUCT_ANE,
	TI_CONSTRUCT_AW,        TI_CONSTRUCT_AS,      TI_CONSTRUCT_AE,
	TI_CONSTRUCT_ASW,       TI_CONSTRUCT_AS,      TI_CONSTRUCT_ASE
	},

	//M_FLUFFBABY
	{
	DONT_USE,               DONT_USE,            DONT_USE,
	DONT_USE,               TI_FLUFFBABY_A,            DONT_USE,
	DONT_USE,               DONT_USE,            DONT_USE
	}
};

const UINT CharacterTileImageArray[CHARACTER_TYPES-CHARACTER_FIRST][ORIENTATION_COUNT] = {
	//nw                    n                       ne
	//w                        none                 e
	//sw                    s                       se

	//M_NEGOTIATOR
	{
	TI_NEGOTIATOR_NW,            TI_NEGOTIATOR_N,      TI_NEGOTIATOR_NE,
	TI_NEGOTIATOR_W,             TI_NEGOTIATOR_S,      TI_NEGOTIATOR_E,
	TI_NEGOTIATOR_SW,            TI_NEGOTIATOR_S,      TI_NEGOTIATOR_SE
	},

	//M_CITIZEN1
	{
	TI_CITIZEN1_NW,            TI_CITIZEN1_N,      TI_CITIZEN1_NE,
	TI_CITIZEN1_W,             TI_CITIZEN1_S,      TI_CITIZEN1_E,
	TI_CITIZEN1_SW,            TI_CITIZEN1_S,      TI_CITIZEN1_SE
	},

	//M_CITIZEN2
	{
	TI_CITIZEN1_NW,            TI_CITIZEN1_N,      TI_CITIZEN1_NE,
	TI_CITIZEN1_W,             TI_CITIZEN1_S,      TI_CITIZEN1_E,
	TI_CITIZEN1_SW,            TI_CITIZEN1_S,      TI_CITIZEN1_SE
	},

	//M_INSTRUCTOR (2.0 deprecated)
	{
	TI_NEGOTIATOR_NW,            TI_NEGOTIATOR_N,      TI_NEGOTIATOR_NE,
	TI_NEGOTIATOR_W,             TI_NEGOTIATOR_S,      TI_NEGOTIATOR_E,
	TI_NEGOTIATOR_SW,            TI_NEGOTIATOR_S,      TI_NEGOTIATOR_SE
	},

	//M_MUDCOORDINATOR (2.0 deprecated)
	{
	TI_CITIZEN1_NW,            TI_CITIZEN1_N,      TI_CITIZEN1_NE,
	TI_CITIZEN1_W,             TI_CITIZEN1_S,      TI_CITIZEN1_E,
	TI_CITIZEN1_SW,            TI_CITIZEN1_S,      TI_CITIZEN1_SE
	},

	//M_TARTECHNICIAN (2.0 deprecated)
	{
	TI_NEGOTIATOR_NW,            TI_NEGOTIATOR_N,      TI_NEGOTIATOR_NE,
	TI_NEGOTIATOR_W,             TI_NEGOTIATOR_S,      TI_NEGOTIATOR_E,
	TI_NEGOTIATOR_SW,            TI_NEGOTIATOR_S,      TI_NEGOTIATOR_SE
	},

	//M_BEETHRO
	{
	TI_SMAN_YNW,              TI_SMAN_YN,          TI_SMAN_YNE,
	TI_SMAN_YW,               TI_SMAN_YS,          TI_SMAN_YE,
	TI_SMAN_YSW,              TI_SMAN_YS,          TI_SMAN_YSE
	},

	//M_CITIZEN3
	{
	TI_CITIZEN1_NW,            TI_CITIZEN1_N,      TI_CITIZEN1_NE,
	TI_CITIZEN1_W,             TI_CITIZEN1_S,      TI_CITIZEN1_E,
	TI_CITIZEN1_SW,            TI_CITIZEN1_S,      TI_CITIZEN1_SE
	},

	//M_CITIZEN4
	{
	TI_CITIZEN1_NW,            TI_CITIZEN1_N,      TI_CITIZEN1_NE,
	TI_CITIZEN1_W,             TI_CITIZEN1_S,      TI_CITIZEN1_E,
	TI_CITIZEN1_SW,            TI_CITIZEN1_S,      TI_CITIZEN1_SE
	},

	//M_BEETHRO_IN_DISGUISE
	{
	TI_GUARD_NW,           TI_GUARD_N,         TI_GUARD_NE,
	TI_GUARD_W,            TI_GUARD_S,         TI_GUARD_E,
	TI_GUARD_SW,           TI_GUARD_S,         TI_GUARD_SE
	},

	//M_STALWART
	{
	TI_STALWART_NW,         TI_STALWART_N,         TI_STALWART_NE,
	TI_STALWART_W,          TI_STALWART_S,         TI_STALWART_E,
	TI_STALWART_SW,         TI_STALWART_S,         TI_STALWART_SE
	},

	//M_ARCHIVIST
	{
	TI_ARCHIVIST_NW,        TI_ARCHIVIST_N,        TI_ARCHIVIST_NE,
	TI_ARCHIVIST_W,         TI_ARCHIVIST_S,        TI_ARCHIVIST_E,
	TI_ARCHIVIST_SW,        TI_ARCHIVIST_S,        TI_ARCHIVIST_SE
	},

	//M_ARCHITECT
	{
	TI_ARCHITECT_NW,            TI_ARCHITECT_N,    TI_ARCHITECT_NE,
	TI_ARCHITECT_W,             TI_ARCHITECT_S,    TI_ARCHITECT_E,
	TI_ARCHITECT_SW,            TI_ARCHITECT_S,    TI_ARCHITECT_SE
	},

	//M_PATRON
	{
	TI_PATRON_NW,            TI_PATRON_N,    TI_PATRON_NE,
	TI_PATRON_W,             TI_PATRON_S,    TI_PATRON_E,
	TI_PATRON_SW,            TI_PATRON_S,    TI_PATRON_SE
	}
};


//Sword tiles.
const UINT SWORD_TI[] = {
	TI_SWORD_YNW, TI_SWORD_YN, TI_SWORD_YNE,
	TI_SWORD_YW,  TI_SWORD_YS, TI_SWORD_YE,
	TI_SWORD_YSW, TI_SWORD_YS, TI_SWORD_YSE};

const UINT SWORDI_TI[] = {
	TI_SWORD_IYNW, TI_SWORD_IYN, TI_SWORD_IYNE,
	TI_SWORD_IYW,  TI_SWORD_IYS, TI_SWORD_IYE,
	TI_SWORD_IYSW, TI_SWORD_IYS, TI_SWORD_IYSE};

const UINT MIMIC_SWORD_TI[] = {
	TI_MIMIC_SWORD_NW, TI_MIMIC_SWORD_N, TI_MIMIC_SWORD_NE,
	TI_MIMIC_SWORD_W,  TI_MIMIC_SWORD_S, TI_MIMIC_SWORD_E,
	TI_MIMIC_SWORD_SW, TI_MIMIC_SWORD_S, TI_MIMIC_SWORD_SE};

const UINT GOBLIN_SWORD_TI[] = {
	TI_GOBLIN_SWORD_NW, TI_GOBLIN_SWORD_N, TI_GOBLIN_SWORD_NE,
	TI_GOBLIN_SWORD_W,  TI_GOBLIN_SWORD_S, TI_GOBLIN_SWORD_E,
	TI_GOBLIN_SWORD_SW, TI_GOBLIN_SWORD_S, TI_GOBLIN_SWORD_SE};

const UINT GUARD_SWORD_TI[] = {
	TI_GUARD_SWORD_NW, TI_GUARD_SWORD_N, TI_GUARD_SWORD_NE,
	TI_GUARD_SWORD_W,  TI_GUARD_SWORD_S, TI_GUARD_SWORD_E,
	TI_GUARD_SWORD_SW, TI_GUARD_SWORD_S, TI_GUARD_SWORD_SE};

const UINT SLAYER_SWORD_TI[] = {
	TI_SLAYER_SWORD_NW, TI_SLAYER_SWORD_N, TI_SLAYER_SWORD_NE,
	TI_SLAYER_SWORD_W,  TI_SLAYER_SWORD_S, TI_SLAYER_SWORD_E,
	TI_SLAYER_SWORD_SW, TI_SLAYER_SWORD_S, TI_SLAYER_SWORD_SE};

const UINT LUCKY_SWORD_TI[] = {
	TI_SWORD7_NW, TI_SWORD7_N, TI_SWORD7_NE,
	TI_SWORD7_W,  TI_SWORD7_S,   TI_SWORD7_E,
	TI_SWORD7_SW, TI_SWORD7_S, TI_SWORD7_SE};

const UINT SERPENT_SWORD_TI[] = {
	TI_SWORD8_NW, TI_SWORD8_N, TI_SWORD8_NE,
	TI_SWORD8_W,  TI_SWORD8_S,   TI_SWORD8_E,
	TI_SWORD8_SW, TI_SWORD8_S, TI_SWORD8_SE};

const UINT BRIAR_SWORD_TI[] = {
	TI_SWORD9_NW, TI_SWORD9_N, TI_SWORD9_NE,
	TI_SWORD9_W,  TI_SWORD9_S,   TI_SWORD9_E,
	TI_SWORD9_SW, TI_SWORD9_S, TI_SWORD9_SE};

const UINT STAFF_SWORD_TI[] = {
	TI_STAFF_SWORD_NW, TI_STAFF_SWORD_N, TI_STAFF_SWORD_NE,
	TI_STAFF_SWORD_W,  TI_STAFF_SWORD_S, TI_STAFF_SWORD_E,
	TI_STAFF_SWORD_SW, TI_STAFF_SWORD_S, TI_STAFF_SWORD_SE };

const UINT SPEAR_SWORD_TI[] = {
	TI_SPEAR_SWORD_NW, TI_SPEAR_SWORD_N, TI_SPEAR_SWORD_NE,
	TI_SPEAR_SWORD_W,  TI_SPEAR_SWORD_S, TI_SPEAR_SWORD_E,
	TI_SPEAR_SWORD_SW, TI_SPEAR_SWORD_S, TI_SPEAR_SWORD_SE };


//Unarmed entity tiles.
const UINT BEETHRO_UTI[] = {
	TI_BEETHRO_UYNW, TI_BEETHRO_UYN, TI_BEETHRO_UYNE,
	TI_BEETHRO_UYW,  TI_BEETHRO_UYS, TI_BEETHRO_UYE,
	TI_BEETHRO_UYSW, TI_BEETHRO_UYS, TI_BEETHRO_UYSE};

const UINT MIMIC_UTI[] = {
	TI_MIMIC_UNW, TI_MIMIC_UN, TI_MIMIC_UNE,
	TI_MIMIC_UW,  TI_MIMIC_US, TI_MIMIC_UE,
	TI_MIMIC_USW, TI_MIMIC_US, TI_MIMIC_USE};

const UINT DECOY_UTI[] = {
	TI_DECOY_UNW, TI_DECOY_UN, TI_DECOY_UNE,
	TI_DECOY_UW,  TI_DECOY_US, TI_DECOY_UE,
	TI_DECOY_USW, TI_DECOY_US, TI_DECOY_USE};

const UINT SLAYER_UTI[] = {
	TI_SLAYER_UNW, TI_SLAYER_UN, TI_SLAYER_UNE,
	TI_SLAYER_UW,  TI_SLAYER_US, TI_SLAYER_UE,
	TI_SLAYER_USW, TI_SLAYER_US, TI_SLAYER_USE};

const UINT GUARD_UTI[] = {
	TI_GUARD_UNW, TI_GUARD_UN, TI_GUARD_UNE,
	TI_GUARD_UW,  TI_GUARD_US, TI_GUARD_UE,
	TI_GUARD_USW, TI_GUARD_US, TI_GUARD_USE};

const UINT PIRATE_UTI[] = {
	TI_PIRATE_UNW, TI_PIRATE_UN, TI_PIRATE_UNE,
	TI_PIRATE_UW,  TI_PIRATE_US, TI_PIRATE_UE,
	TI_PIRATE_USW, TI_PIRATE_US, TI_PIRATE_USE};

const UINT STALWART_UTI[] = {
	TI_STALWART_UNW, TI_STALWART_UN, TI_STALWART_UNE,
	TI_STALWART_UW,  TI_STALWART_US, TI_STALWART_UE,
	TI_STALWART_USW, TI_STALWART_US, TI_STALWART_USE};

//This array defines the set of scenery obstacles available,
//indexed by type and size variants.
//Each value is an index into the obstacleTile array below.
//The size of each of these obstacles is listed in 'obstacleDimensions' below.
const UINT obstacleIndices[MAX_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE] = {
	{0},  //0th index signifies an invalid obstacle type
	{1, 2, 3},     //boulder
	{4, 5, 6},
	{7, 8},        //fern
	{9, 10},
	{11, 12, 13},  //building
	{14, 15, 16},
	{17, 18},      //skulls
	{19, 20},
	{21, 22, 23},  //Goblin statue
	{24, 25, 26},  //Empire statue
	{27, 28},      //desk
	{29, 30},      //bookshelf
	{31},          //chairs
	{32},
	{33},
	{34},
	{35},          //book pedestal
	{36},          //chest
	{37},          //graves
	{38},
	{39},          //crate
	{40, 41, 42},  //table
	{43, 44},      //table w/ runner
	{45, 46},      //clockweight
	{47, 48},      //hut
	{49, 50},      //cauldron
	{51, 52},      //bed
	{53},          //sign post
	{54},          //barrel
	{55, 59},      //pipe
	{56, 57},      //couches
	{58}           //washbasin
};

//These (x,y) pairs indicate the dimensions of each obstacle, in order of
//index specified in 'obstacleIndices' above.
const UINT obstacleDimensions[TOTAL_OBSTACLE_TYPES][2] = {
	{0},
	{1,1},{2,2},{3,3},  //boulder
	{1,1},{2,2},{3,3},
	{1,1},{2,2},        //fern
	{1,1},{2,2},
	{1,1},{3,3},{5,5},  //building
	{1,1},{3,3},{5,5},
	{1,1},{2,2},        //skulls
	{1,1},{2,2},
	{1,1},{2,2},{4,4},  //Goblin statue
	{1,1},{2,2},{4,4},  //Empire statue
	{1,1},{3,3},        //desk
	{1,1},{2,1},        //bookshelf
	{1,1},              //chairs
	{1,1},
	{1,1},
	{1,1},
	{1,1},              //book pedestal
	{1,1},              //chest
	{1,1},              //graves
	{1,1},
	{1,1},              //crate
	{1,1},{2,2},{3,3},  //table
	{1,1},{2,2},        //table w/ runner
	{1,1},{4,4},        //clockweight
	{1,1},{3,3},        //hut
	{1,1},{2,2},        //cauldron
	{1,1},{1,2},        //bed
	{1,1},              //sign post
	{1,1},              //barrel
	{1,1},              //pipe
	{1,1},{2,1},        //couches
	{1,1},              //washbasin
	{3,1}               //pipe (3x1)
};

//Tiles used for defined obstacles.
const UINT obstacleTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE][MAX_OBSTACLE_SIZE] = {
	{{0},{0}},  //0th index signifies an invalid obstacle type

	{{TI_OB_1_1}},
	{{TI_OB_1_2NW,TI_OB_1_2SW},{TI_OB_1_2NE,TI_OB_1_2SE}},
	{{TI_OB_1_3NW,TI_OB_1_3W,TI_OB_1_3SW},
	 {TI_OB_1_3N,TI_OB_1_3,TI_OB_1_3S},
	 {TI_OB_1_3NE,TI_OB_1_3E,TI_OB_1_3SE}},

	{{TI_OB_2_1}},
	{{TI_OB_2_2NW,TI_OB_2_2SW},{TI_OB_2_2NE,TI_OB_2_2SE}},
	{{TI_OB_2_3NW,TI_OB_2_3W,TI_OB_2_3SW},
	 {TI_OB_2_3N,TI_OB_2_3,TI_OB_2_3S},
	 {TI_OB_2_3NE,TI_OB_2_3E,TI_OB_2_3SE}},

	{{TI_OB_3_1}},
	{{TI_OB_3_2NW,TI_OB_3_2SW},{TI_OB_3_2NE,TI_OB_3_2SE}},

	{{TI_OB_4_1}},
	{{TI_OB_4_2NW,TI_OB_4_2SW},{TI_OB_4_2NE,TI_OB_4_2SE}},

	{{TI_OB_5_1}},
	{{TI_OB_5_3_11,TI_OB_5_3_12,TI_OB_5_3_13},
	 {TI_OB_5_3_21,TI_OB_5_3_22,TI_OB_5_3_23},
	 {TI_OB_5_3_31,TI_OB_5_3_32,TI_OB_5_3_33}},
	{{TI_OB_5_5_11,TI_OB_5_5_12,TI_OB_5_5_13,TI_OB_5_5_14,TI_OB_5_5_15},
	 {TI_OB_5_5_21,TI_OB_5_5_22,TI_OB_5_5_23,TI_OB_5_5_24,TI_OB_5_5_25},
	 {TI_OB_5_5_31,TI_OB_5_5_32,TI_OB_5_5_33,TI_OB_5_5_34,TI_OB_5_5_35},
	 {TI_OB_5_5_41,TI_OB_5_5_42,TI_OB_5_5_43,TI_OB_5_5_44,TI_OB_5_5_45},
	 {TI_OB_5_5_51,TI_OB_5_5_52,TI_OB_5_5_53,TI_OB_5_5_54,TI_OB_5_5_55}},

	{{TI_OB_6_1}},
	{{TI_OB_6_3_11,TI_OB_6_3_12,TI_OB_6_3_13},
	 {TI_OB_6_3_21,TI_OB_6_3_22,TI_OB_6_3_23},
	 {TI_OB_6_3_31,TI_OB_6_3_32,TI_OB_6_3_33}},
	{{TI_OB_6_5_11,TI_OB_6_5_12,TI_OB_6_5_13,TI_OB_6_5_14,TI_OB_6_5_15},
	 {TI_OB_6_5_21,TI_OB_6_5_22,TI_OB_6_5_23,TI_OB_6_5_24,TI_OB_6_5_25},
	 {TI_OB_6_5_31,TI_OB_6_5_32,TI_OB_6_5_33,TI_OB_6_5_34,TI_OB_6_5_35},
	 {TI_OB_6_5_41,TI_OB_6_5_42,TI_OB_6_5_43,TI_OB_6_5_44,TI_OB_6_5_45},
	 {TI_OB_6_5_51,TI_OB_6_5_52,TI_OB_6_5_53,TI_OB_6_5_54,TI_OB_6_5_55}},

	{{TI_OB_7_1}},
	{{TI_OB_7_2NW,TI_OB_7_2SW},{TI_OB_7_2NE,TI_OB_7_2SE}},

	{{TI_OB_8_1}},
	{{TI_OB_8_2NW,TI_OB_8_2SW},{TI_OB_8_2NE,TI_OB_8_2SE}},

	{{TI_OB_9_1}},
	{{TI_OB_9_2_11,TI_OB_9_2_12},{TI_OB_9_2_21,TI_OB_9_2_22}},
	{{TI_OB_9_4_11,TI_OB_9_4_12,TI_OB_9_4_13,TI_OB_9_4_14},
	 {TI_OB_9_4_21,TI_OB_9_4_22,TI_OB_9_4_23,TI_OB_9_4_24},
	 {TI_OB_9_4_31,TI_OB_9_4_32,TI_OB_9_4_33,TI_OB_9_4_34},
	 {TI_OB_9_4_41,TI_OB_9_4_42,TI_OB_9_4_43,TI_OB_9_4_44}},

	{{TI_OB_10_1}},
	{{TI_OB_10_2_11,TI_OB_10_2_12},{TI_OB_10_2_21,TI_OB_10_2_22}},
	{{TI_OB_10_4_11,TI_OB_10_4_12,TI_OB_10_4_13,TI_OB_10_4_14},
	 {TI_OB_10_4_21,TI_OB_10_4_22,TI_OB_10_4_23,TI_OB_10_4_24},
	 {TI_OB_10_4_31,TI_OB_10_4_32,TI_OB_10_4_33,TI_OB_10_4_34},
	 {TI_OB_10_4_41,TI_OB_10_4_42,TI_OB_10_4_43,TI_OB_10_4_44}},

	{{TI_OB_11_1}},
	{{TI_OB_11_3_11,TI_OB_11_3_12,TI_OB_11_3_13},
	 {TI_OB_11_3_21,TI_OB_11_3_22,TI_OB_11_3_23},
	 {TI_OB_11_3_31,TI_OB_11_3_32,TI_OB_11_3_33}},

	{{TI_OB_12_1}},
	{{TI_OB_12_2W},{TI_OB_12_2E}},

	{{TI_OB_13_1}},

	{{TI_OB_14_1}},

	{{TI_OB_15_1}},

	{{TI_OB_16_1}},

	{{TI_OB_17_1}},

	{{TI_OB_18_1}},

	{{TI_OB_19_1}},

	{{TI_OB_20_1}},

	{{TI_OB_21_1}},

	{{TI_OB_22_1}},
	{{TI_OB_22_2_11,TI_OB_22_2_12},{TI_OB_22_2_21,TI_OB_22_2_22}},
	{{TI_OB_22_3_11,TI_OB_22_3_12,TI_OB_22_3_13},
	 {TI_OB_22_3_21,TI_OB_22_3_22,TI_OB_22_3_23},
	 {TI_OB_22_3_31,TI_OB_22_3_32,TI_OB_22_3_33}},

	{{TI_OB_23_1}},
	{{TI_OB_23_2_11,TI_OB_23_2_12},{TI_OB_23_2_21,TI_OB_23_2_22}},

	{{TI_OB_24_1}},
	{{TI_OB_24_4_11,TI_OB_24_4_12,TI_OB_24_4_13,TI_OB_24_4_14},
	 {TI_OB_24_4_21,TI_OB_24_4_22,TI_OB_24_4_23,TI_OB_24_4_24},
	 {TI_OB_24_4_31,TI_OB_24_4_32,TI_OB_24_4_33,TI_OB_24_4_34},
	 {TI_OB_24_4_41,TI_OB_24_4_42,TI_OB_24_4_43,TI_OB_24_4_44}},

	{{TI_OB_25_1}},
	{{TI_OB_25_3_11,TI_OB_25_3_12,TI_OB_25_3_13},
	 {TI_OB_25_3_21,TI_OB_25_3_22,TI_OB_25_3_23},
	 {TI_OB_25_3_31,TI_OB_25_3_32,TI_OB_25_3_33}},

	{{TI_OB_26_1}},
	{{TI_OB_26_2_11,TI_OB_26_2_12},{TI_OB_26_2_21,TI_OB_26_2_22}},

	{{TI_OB_27_1}},
	{{TI_OB_27_2_N,TI_OB_27_2_S}},

	{{TI_OB_28_1}},

	{{TI_OB_29_1}},

	{{TI_OB_30_1}},

	{{TI_OB_31_1}},
	{{TI_OB_31_2W},{TI_OB_31_2E}},

	{{TI_OB_32_1}},

	{{TI_OB_30_3W},{TI_OB_30_3C},{TI_OB_30_3E}}
};

//Shadows cast from obstacles.
const UINT obstacleShadowTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE+1][MAX_OBSTACLE_SIZE+1] = {
	{{0},{0}},  //0th index signifies an invalid obstacle type
	{{TI_OBS_1_1_11,TI_OBS_1_1_12},{TI_OBS_1_1_21,TI_OBS_1_1_22}},
	{{TI_OBS_1_2_11,TI_OBS_1_2_12,TI_OBS_1_2_13},
	 {TI_OBS_1_2_21,TI_OBS_1_2_22,TI_OBS_1_2_23},
	 {TI_OBS_1_2_31,TI_OBS_1_2_32,TI_OBS_1_2_33}},
	{{TI_OBS_1_3_11,TI_OBS_1_3_12,TI_OBS_1_3_13,TI_OBS_1_3_14},
	 {TI_OBS_1_3_21,TI_OBS_1_3_22,TI_OBS_1_3_23,TI_OBS_1_3_24},
	 {TI_OBS_1_3_31,TI_OBS_1_3_32,TI_OBS_1_3_33,TI_OBS_1_3_34},
	 {TI_OBS_1_3_41,TI_OBS_1_3_42,TI_OBS_1_3_43,TI_OBS_1_3_44}},

	{{TI_OBS_2_1_11,TI_OBS_2_1_12},{TI_OBS_2_1_21,TI_OBS_2_1_22}},
	{{TI_OBS_2_2_11,TI_OBS_2_2_12,TI_OBS_2_2_13},
	 {TI_OBS_2_2_21,TI_OBS_2_2_22,TI_OBS_2_2_23},
	 {TI_OBS_2_2_31,TI_OBS_2_2_32,TI_OBS_2_2_33}},
	{{TI_OBS_2_3_11,TI_OBS_2_3_12,TI_OBS_2_3_13,TI_OBS_2_3_14},
	 {TI_OBS_2_3_21,TI_OBS_2_3_22,TI_OBS_2_3_23,TI_OBS_2_3_24},
	 {TI_OBS_2_3_31,TI_OBS_2_3_32,TI_OBS_2_3_33,TI_OBS_2_3_34},
	 {TI_OBS_2_3_41,TI_OBS_2_3_42,TI_OBS_2_3_43,TI_OBS_2_3_44}},

	{{TI_OBS_3_1_11,TI_OBS_3_1_12},{TI_OBS_3_1_21,TI_OBS_3_1_22}},
	{{TI_OBS_3_2_11,TI_OBS_3_2_12,TI_OBS_3_2_13},
	 {TI_OBS_3_2_21,TI_OBS_3_2_22,TI_OBS_3_2_23},
	 {TI_OBS_3_2_31,TI_OBS_3_2_32,TI_OBS_3_2_33}},

	{{TI_OBS_4_1_11,TI_OBS_4_1_12},{TI_OBS_4_1_21,TI_OBS_4_1_22}},
	{{TI_OBS_4_2_11,TI_OBS_4_2_12,TI_OBS_4_2_13},
	 {TI_OBS_4_2_21,TI_OBS_4_2_22,TI_OBS_4_2_23},
	 {TI_OBS_4_2_31,TI_OBS_4_2_32,TI_OBS_4_2_33}},

	{{TI_OBS_5_1_11,TI_OBS_5_1_12},{TI_OBS_5_1_21,TI_OBS_5_1_22}},
	{{TI_OBS_5_3_11,TI_OBS_5_3_12,TI_OBS_5_3_13,TI_OBS_5_3_14},
	 {TI_OBS_5_3_21,TI_OBS_5_3_22,TI_OBS_5_3_23,TI_OBS_5_3_24},
	 {TI_OBS_5_3_31,TI_OBS_5_3_32,TI_OBS_5_3_33,TI_OBS_5_3_34},
	 {TI_OBS_5_3_41,TI_OBS_5_3_42,TI_OBS_5_3_43,TI_OBS_5_3_44}},
	{{TI_OBS_5_5_11,TI_OBS_5_5_12,TI_OBS_5_5_13,TI_OBS_5_5_14,TI_OBS_5_5_15,TI_OBS_5_5_16},
	 {TI_OBS_5_5_21,TI_OBS_5_5_22,TI_OBS_5_5_23,TI_OBS_5_5_24,TI_OBS_5_5_25,TI_OBS_5_5_26},
	 {TI_OBS_5_5_31,TI_OBS_5_5_32,TI_OBS_5_5_33,TI_OBS_5_5_34,TI_OBS_5_5_35,TI_OBS_5_5_36},
	 {TI_OBS_5_5_41,TI_OBS_5_5_42,TI_OBS_5_5_43,TI_OBS_5_5_44,TI_OBS_5_5_45,TI_OBS_5_5_46},
	 {TI_OBS_5_5_51,TI_OBS_5_5_52,TI_OBS_5_5_53,TI_OBS_5_5_54,TI_OBS_5_5_55,TI_OBS_5_5_56},
	 {TI_OBS_5_5_61,TI_OBS_5_5_62,TI_OBS_5_5_63,TI_OBS_5_5_64,TI_OBS_5_5_65,TI_OBS_5_5_66}},

	{{TI_OBS_6_1_11,TI_OBS_6_1_12},{TI_OBS_6_1_21,TI_OBS_6_1_22}},
	{{TI_OBS_6_3_11,TI_OBS_6_3_12,TI_OBS_6_3_13,TI_OBS_6_3_14},
	 {TI_OBS_6_3_21,TI_OBS_6_3_22,TI_OBS_6_3_23,TI_OBS_6_3_24},
	 {TI_OBS_6_3_31,TI_OBS_6_3_32,TI_OBS_6_3_33,TI_OBS_6_3_34},
	 {TI_OBS_6_3_41,TI_OBS_6_3_42,TI_OBS_6_3_43,TI_OBS_6_3_44}},
	{{TI_OBS_6_5_11,TI_OBS_6_5_12,TI_OBS_6_5_13,TI_OBS_6_5_14,TI_OBS_6_5_15,TI_OBS_6_5_16},
	 {TI_OBS_6_5_21,TI_OBS_6_5_22,TI_OBS_6_5_23,TI_OBS_6_5_24,TI_OBS_6_5_25,TI_OBS_6_5_26},
	 {TI_OBS_6_5_31,TI_OBS_6_5_32,TI_OBS_6_5_33,TI_OBS_6_5_34,TI_OBS_6_5_35,TI_OBS_6_5_36},
	 {TI_OBS_6_5_41,TI_OBS_6_5_42,TI_OBS_6_5_43,TI_OBS_6_5_44,TI_OBS_6_5_45,TI_OBS_6_5_46},
	 {TI_OBS_6_5_51,TI_OBS_6_5_52,TI_OBS_6_5_53,TI_OBS_6_5_54,TI_OBS_6_5_55,TI_OBS_6_5_56},
	 {TI_OBS_6_5_61,TI_OBS_6_5_62,TI_OBS_6_5_63,TI_OBS_6_5_64,TI_OBS_6_5_65,TI_OBS_6_5_66}},

	{{TI_OBS_7_1_11,TI_OBS_7_1_12},{TI_OBS_7_1_21,TI_OBS_7_1_22}},
	{{TI_OBS_7_2_11,TI_OBS_7_2_12,TI_OBS_7_2_13},
	 {TI_OBS_7_2_21,TI_OBS_7_2_22,TI_OBS_7_2_23},
	 {TI_OBS_7_2_31,TI_OBS_7_2_32,TI_OBS_7_2_33}},

	{{TI_OBS_8_1_11,TI_OBS_8_1_12},{TI_OBS_8_1_21,TI_OBS_8_1_22}},
	{{TI_OBS_8_2_11,TI_OBS_8_2_12,TI_OBS_8_2_13},
	 {TI_OBS_8_2_21,TI_OBS_8_2_22,TI_OBS_8_2_23},
	 {TI_OBS_8_2_31,TI_OBS_8_2_32,TI_OBS_8_2_33}},

	{{TI_OBS_9_1_11,TI_OBS_9_1_12},{TI_OBS_9_1_21,TI_OBS_9_1_22}},
	{{TI_OBS_9_2_11,TI_OBS_9_2_12,TI_OBS_9_2_13},
	 {TI_OBS_9_2_21,TI_OBS_9_2_22,TI_OBS_9_2_23},
	 {TI_OBS_9_2_31,TI_OBS_9_2_32,TI_OBS_9_2_33}},
	{{TI_OBS_9_4_11,TI_OBS_9_4_12,TI_OBS_9_4_13,TI_OBS_9_4_14,TI_OBS_9_4_15},
	 {TI_OBS_9_4_21,TI_OBS_9_4_22,TI_OBS_9_4_23,TI_OBS_9_4_24,TI_OBS_9_4_25},
	 {TI_OBS_9_4_31,TI_OBS_9_4_32,TI_OBS_9_4_33,TI_OBS_9_4_34,TI_OBS_9_4_35},
	 {TI_OBS_9_4_41,TI_OBS_9_4_42,TI_OBS_9_4_43,TI_OBS_9_4_44,TI_OBS_9_4_45},
	 {TI_OBS_9_4_51,TI_OBS_9_4_52,TI_OBS_9_4_53,TI_OBS_9_4_54,TI_OBS_9_4_55}},

	{{TI_OBS_10_1_11,TI_OBS_10_1_12},{TI_OBS_10_1_21,TI_OBS_10_1_22}},
	{{TI_OBS_10_2_11,TI_OBS_10_2_12,TI_OBS_10_2_13},
	 {TI_OBS_10_2_21,TI_OBS_10_2_22,TI_OBS_10_2_23},
	 {TI_OBS_10_2_31,TI_OBS_10_2_32,TI_OBS_10_2_33}},
	{{TI_OBS_10_4_11,TI_OBS_10_4_12,TI_OBS_10_4_13,TI_OBS_10_4_14,TI_OBS_10_4_15},
	 {TI_OBS_10_4_21,TI_OBS_10_4_22,TI_OBS_10_4_23,TI_OBS_10_4_24,TI_OBS_10_4_25},
	 {TI_OBS_10_4_31,TI_OBS_10_4_32,TI_OBS_10_4_33,TI_OBS_10_4_34,TI_OBS_10_4_35},
	 {TI_OBS_10_4_41,TI_OBS_10_4_42,TI_OBS_10_4_43,TI_OBS_10_4_44,TI_OBS_10_4_45},
	 {TI_OBS_10_4_51,TI_OBS_10_4_52,TI_OBS_10_4_53,TI_OBS_10_4_54,TI_OBS_10_4_55}},

	{{TI_OBS_11_1_11,TI_OBS_11_1_12},{TI_OBS_11_1_21,TI_OBS_11_1_22}},
	{{TI_OBS_11_3_11,TI_OBS_11_3_12,TI_OBS_11_3_13,TI_OBS_11_3_14},
	 {TI_OBS_11_3_21,TI_OBS_11_3_22,TI_OBS_11_3_23,TI_OBS_11_3_24},
	 {TI_OBS_11_3_31,TI_OBS_11_3_32,TI_OBS_11_3_33,TI_OBS_11_3_34},
	 {TI_OBS_11_3_41,TI_OBS_11_3_42,TI_OBS_11_3_43,TI_OBS_11_3_44}},

	{{TI_OBS_12_1_11,TI_OBS_12_1_12},{TI_OBS_12_1_21,TI_OBS_12_1_22}},
	{{TI_OBS_12_2_11,TI_OBS_12_2_12},
	 {TI_OBS_12_2_21,TI_OBS_12_2_22},
	 {TI_OBS_12_2_31,TI_OBS_12_2_32}},

	{{TI_OBS_13_1_11,TI_OBS_13_1_12},{TI_OBS_13_1_21,TI_OBS_13_1_22}},

	{{TI_OBS_14_1_11,TI_OBS_14_1_12},{TI_OBS_14_1_21,TI_OBS_14_1_22}},

	{{TI_OBS_15_1_11,TI_OBS_15_1_12},{TI_OBS_15_1_21,TI_OBS_15_1_22}},

	{{TI_OBS_16_1_11,TI_OBS_16_1_12},{TI_OBS_16_1_21,TI_OBS_16_1_22}},

	{{TI_OBS_17_1_11,TI_OBS_17_1_12},{TI_OBS_17_1_21,TI_OBS_17_1_22}},

	{{TI_OBS_18_1_11,TI_OBS_18_1_12},{TI_OBS_18_1_21,TI_OBS_18_1_22}},

	{{TI_OBS_19_1_11,TI_OBS_19_1_12},{TI_OBS_19_1_21,TI_OBS_19_1_22}},

	{{TI_OBS_20_1_11,TI_OBS_20_1_12},{TI_OBS_20_1_21,TI_OBS_20_1_22}},

	{{TI_OBS_21_1_11,TI_OBS_21_1_12},{TI_OBS_21_1_21,TI_OBS_21_1_22}},

	{{TI_OBS_22_1_11,TI_OBS_22_1_12},{TI_OBS_22_1_21,TI_OBS_22_1_22}},
	{{TI_OBS_22_2_11,TI_OBS_22_2_12,TI_OBS_22_2_13},
	 {TI_OBS_22_2_21,TI_OBS_22_2_22,TI_OBS_22_2_23},
	 {TI_OBS_22_2_31,TI_OBS_22_2_32,TI_OBS_22_2_33}},
	{{TI_OBS_22_3_11,TI_OBS_22_3_12,TI_OBS_22_3_13,TI_OBS_22_3_14},
	 {TI_OBS_22_3_21,TI_OBS_22_3_22,TI_OBS_22_3_23,TI_OBS_22_3_24},
	 {TI_OBS_22_3_31,TI_OBS_22_3_32,TI_OBS_22_3_33,TI_OBS_22_3_34},
	 {TI_OBS_22_3_41,TI_OBS_22_3_42,TI_OBS_22_3_43,TI_OBS_22_3_44}},

	{{TI_OBS_23_1_11,TI_OBS_23_1_12},{TI_OBS_23_1_21,TI_OBS_23_1_22}},
	{{TI_OBS_23_2_11,TI_OBS_23_2_12,TI_OBS_23_2_13},
	 {TI_OBS_23_2_21,TI_OBS_23_2_22,TI_OBS_23_2_23},
	 {TI_OBS_23_2_31,TI_OBS_23_2_32,TI_OBS_23_2_33}},

	{{TI_OBS_24_1_11,TI_OBS_24_1_12},{TI_OBS_24_1_21,TI_OBS_24_1_22}},
	{{TI_OBS_24_4_11,TI_OBS_24_4_12,TI_OBS_24_4_13,TI_OBS_24_4_14,TI_OBS_24_4_15},
	 {TI_OBS_24_4_21,TI_OBS_24_4_22,TI_OBS_24_4_23,TI_OBS_24_4_24,TI_OBS_24_4_25},
	 {TI_OBS_24_4_31,TI_OBS_24_4_32,TI_OBS_24_4_33,TI_OBS_24_4_34,TI_OBS_24_4_35},
	 {TI_OBS_24_4_41,TI_OBS_24_4_42,TI_OBS_24_4_43,TI_OBS_24_4_44,TI_OBS_24_4_45},
	 {TI_OBS_24_4_51,TI_OBS_24_4_52,TI_OBS_24_4_53,TI_OBS_24_4_54,TI_OBS_24_4_55}},

	{{TI_OBS_25_1_11,TI_OBS_25_1_12},{TI_OBS_25_1_21,TI_OBS_25_1_22}},
	{{TI_OBS_25_3_11,TI_OBS_25_3_12,TI_OBS_25_3_13,TI_OBS_25_3_14},
	 {TI_OBS_25_3_21,TI_OBS_25_3_22,TI_OBS_25_3_23,TI_OBS_25_3_24},
	 {TI_OBS_25_3_31,TI_OBS_25_3_32,TI_OBS_25_3_33,TI_OBS_25_3_34},
	 {TI_OBS_25_3_41,TI_OBS_25_3_42,TI_OBS_25_3_43,TI_OBS_25_3_44}},

	{{TI_OBS_26_1_11,TI_OBS_26_1_12},{TI_OBS_26_1_21,TI_OBS_26_1_22}},
	{{TI_OBS_26_2_11,TI_OBS_26_2_12,TI_OBS_26_2_13},
	 {TI_OBS_26_2_21,TI_OBS_26_2_22,TI_OBS_26_2_23},
	 {TI_OBS_26_2_31,TI_OBS_26_2_32,TI_OBS_26_2_33}},

	{{TI_OBS_27_1_11,TI_OBS_27_1_12},{TI_OBS_27_1_21,TI_OBS_27_1_22}},
	{{TI_OBS_27_2_11,TI_OBS_27_2_12,TI_OBS_27_2_13},
	 {TI_OBS_27_2_21,TI_OBS_27_2_22,TI_OBS_27_2_23}},

	{{TI_OBS_28_1_11,TI_OBS_28_1_12},{TI_OBS_28_1_21,TI_OBS_28_1_22}},

	{{TI_OBS_29_1_11,TI_OBS_29_1_12},{TI_OBS_29_1_21,TI_OBS_29_1_22}},

	{{TI_OBS_30_1_11,TI_OBS_30_1_12},{TI_OBS_30_1_21,TI_OBS_30_1_22}},

	{{TI_OBS_31_1_11,TI_OBS_31_1_12},{TI_OBS_31_1_21,TI_OBS_31_1_22}},
	{{TI_OBS_31_2_11,TI_OBS_31_2_12},
	 {TI_OBS_31_2_21,TI_OBS_31_2_22},
	 {TI_OBS_31_2_31,TI_OBS_31_2_32}},

	{{TI_OBS_32_1_11,TI_OBS_32_1_12},{TI_OBS_32_1_21,TI_OBS_32_1_22}},

	{{TI_OBS_30_3_11,TI_OBS_30_3_12},
	 {TI_OBS_30_3_21,TI_OBS_30_3_22},
	 {TI_OBS_30_3_31,TI_OBS_30_3_32},
	 {TI_OBS_30_3_41,TI_OBS_30_3_42}}
};


//*****************************************************************************
UINT pseudoRandomTileFrame(
//Returns: pseudo-random, deterministic value based on room position
	const CDbRoom *pRoom,
	const UINT col, const UINT row, //room position
	const UINT mod) //number of choices
{
	ASSERT(mod > 0);

	UINT uTileIndex = col*pRoom->wRoomRows + row;
	uTileIndex *= uTileIndex; //squared
	UINT bits=0; //number of bits in tile index
	while (uTileIndex)
	{
		if (uTileIndex & 0x1)
			++bits;
		uTileIndex>>=1;
	}
	return bits % mod;
}

//*****************************************************************************
EDGETYPE CalcEdge(
//Returns:
//True if a black edge should be drawn between these two tiles, else false.
//
// 1. Wall (normal/broken/hidden) squares need to be outlined around their edges
//    to show division between wall squares and non-wall squares.
// 2. Pit squares need to be outlined around their edges
//    to show division between pit and non-pit squares.
//
//Params:
	const UINT wTileNo, const UINT wAdjTileNo,   //(in)
	const UINT side)  //(in) To prevent some sides from being edged for aesthetic reasons
{
	if (wAdjTileNo == wTileNo)
		return EDGE_NONE; //no edging between like tiles

	switch (wTileNo)
	{
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_IMAGE:
		case T_FLOOR_ROAD: case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
		case T_HOT: case T_PRESSPLATE: case T_GOO: case T_MISTVENT:
			if (bIsWall(wAdjTileNo) || bIsCrumblyWall(wAdjTileNo) ||
					bIsDoor(wAdjTileNo) || bIsOpenDoor(wAdjTileNo) ||
					wAdjTileNo==T_OBSTACLE || bIsFallingTile(wAdjTileNo) ||
					wAdjTileNo==T_PRESSPLATE || bIsTunnel(wAdjTileNo) ||
					wAdjTileNo==T_WATER || wAdjTileNo==T_GOO)
				return EDGE_NONE;
			if (bIsPit(wAdjTileNo) || bIsPlatform(wAdjTileNo) || bIsBridge(wAdjTileNo))
				return EDGE_NONE;
			if (!bIsFloor(wAdjTileNo))
				return EDGE_WALL;
			if (wTileNo == T_HOT)
				return EDGE_NONE;
			if (wAdjTileNo==T_HOT)
				return (side == N || side == W) ? EDGE_WALL : EDGE_NONE; //hot tiles have edging on N+W sides
			if (wTileNo == T_PRESSPLATE)
				return EDGE_WALL;
			if ((side == N || side == W) &&
					wTileNo != T_FLOOR_IMAGE && wAdjTileNo != T_FLOOR_IMAGE &&
					wTileNo != T_GOO && wAdjTileNo != T_GOO && wAdjTileNo != T_MISTVENT)
				return EDGE_FLOOR;
			return EDGE_NONE;

		case T_PIT: case T_PIT_IMAGE: case T_PLATFORM_P:
			if (bIsPit(wAdjTileNo) || wAdjTileNo == T_PLATFORM_P ||
					bIsWall(wAdjTileNo) || bIsCrumblyWall(wAdjTileNo) || //walls already have edging
					(wAdjTileNo == T_TRAPDOOR /*&& side == N*/))
				return EDGE_NONE;
			return EDGE_WALL;

		case T_WATER:
			if (bIsFloor(wAdjTileNo) || bIsWall(wAdjTileNo) || bIsCrumblyWall(wAdjTileNo) ||
					bIsPit(wAdjTileNo) || bIsPlatform(wAdjTileNo))
				return EDGE_NONE;
			return EDGE_WALL;

		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
		case T_WALL_B: case T_WALL_H:
			//Wall tiles have pre-drawn edging.
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
		default:
			return EDGE_NONE;
	}
}

//*****************************************************************************
UINT GetSwordlessEntityTile(
//Returns: TI_*constant of a sword-wielding entity when they aren't wielding a sword,
//         or 0 if entity type doesn't have a sword.
//
//Params:
	const UINT wType, //(in)   Contains data needed to figure out
	const UINT wO)
{
	switch (wType)
	{
		case M_BEETHRO_IN_DISGUISE: case M_CLONE:
		case M_BEETHRO: return BEETHRO_UTI[wO];
		case M_DECOY: return DECOY_UTI[wO];
		case M_PIRATE: return PIRATE_UTI[wO];
		case M_STALWART: return STALWART_UTI[wO];
		case M_GUARD: return GUARD_UTI[wO];
		case M_SLAYER: return SLAYER_UTI[wO];
		case M_MIMIC: return MIMIC_UTI[wO];
		default: return 0;
	}
}

//*****************************************************************************
UINT GetPredefinedSwordTile(
//Returns: TI_*constant of a sword tile based sword type carried by entity type.
//
//Params:
	const UINT wType, //(in)   Contains data needed to figure out
	const UINT wO,
	const UINT sword) //specific sword type [default=NoSword]
{
	switch (sword)
	{
		case WoodenBlade: return SWORDI_TI[wO];
		case ShortSword: return GUARD_SWORD_TI[wO];
		case GoblinSword: return GOBLIN_SWORD_TI[wO];
		case LongSword: return MIMIC_SWORD_TI[wO];
		case HookSword: return SLAYER_SWORD_TI[wO];
		case ReallyBigSword: return SWORD_TI[wO];
		case LuckySword: return LUCKY_SWORD_TI[wO];
		case SerpentSword: return SERPENT_SWORD_TI[wO];
		case BriarSword: return BRIAR_SWORD_TI[wO];
		case Staff: return STAFF_SWORD_TI[wO];
		case Spear: return SPEAR_SWORD_TI[wO];
		case WeaponSlot:
		case Dagger:
		default: break; //resolve below
	}

	switch (wType)
	{
		case M_BEETHRO_IN_DISGUISE: case M_CLONE:
		case M_BEETHRO: return SWORD_TI[wO];
		case M_DECOY: return SWORDI_TI[wO];
		case M_PIRATE: case M_STALWART:
		case M_GUARD: return GUARD_SWORD_TI[wO];
		case M_SLAYER: return SLAYER_SWORD_TI[wO];
		case M_MIMIC:
		default:	return MIMIC_SWORD_TI[wO];
	}
}

//*****************************************************************************
UINT GetTileImageForEntity(
//Gets a tile image to display for a monster.
//
//Params:
	const UINT wType, //(in)   Contains data needed to figure out
	const UINT wO,
	const UINT wAnimFrame)     //(in)  Which array to get tile from
								//    tile image.
//
//Returns:
//TI_* constant or CALC_NEEDED if tile needs to be calculated based on
//other context.
{
	ASSERT(IsValidOrientation(wO));
	if (IsValidMonsterType(wType))
	{
		ASSERT(MonsterTileImageArray[wType][wO] != DONT_USE);
		switch (wAnimFrame)
		{
			case SWORDLESS_FRAME:
			{
				const UINT tile = GetSwordlessEntityTile(wType, wO);
				if (tile)
					return tile;
			}
			//no break
			default: 
			case 0: return MonsterTileImageArray[wType][wO];
			case 1: return AnimatedMonsterTileImageArray[wType][wO];
		}
	} else {
		//Character monster pseudo-type.
		ASSERT(wType>=CHARACTER_FIRST && wType<CHARACTER_TYPES);
		ASSERT(CharacterTileImageArray[wType-CHARACTER_FIRST][wO] != DONT_USE);
		if (wAnimFrame == SWORDLESS_FRAME)
		{
			const UINT tile = GetSwordlessEntityTile(wType, wO);
			return tile ? tile : CharacterTileImageArray[wType-CHARACTER_FIRST][wO];
		}
		return CharacterTileImageArray[wType-CHARACTER_FIRST][wO];
	}
}

//*****************************************************************************
UINT GetTileImageForSerpentPiece(
//Gets a tile image to display for a monster.
//
//Params:
	const UINT wType,    //(in)   Monster type
	const UINT wTileNo)  //(in)   Abstract tile type
{
	static const UINT SerpentPieceImageArray[3][10] =
	{
		{
			TI_SNK_EW,     //T_SNK_EW
			TI_SNK_NS,     //T_SNK_NS
			TI_SNK_NW,     //T_SNK_NW
			TI_SNK_NE,     //T_SNK_NE
			TI_SNK_SW,     //T_SNK_SW
			TI_SNK_SE,     //T_SNK_SE
			TI_SNKT_S,     //T_SNKT_S
			TI_SNKT_W,     //T_SNKT_W
			TI_SNKT_N,     //T_SNKT_N
			TI_SNKT_E      //T_SNKT_E
		},
		{
			TI_SNK_B_EW,      //T_SNK_EW
			TI_SNK_B_NS,      //T_SNK_NS
			TI_SNK_B_NW,      //T_SNK_NW
			TI_SNK_B_NE,      //T_SNK_NE
			TI_SNK_B_SW,      //T_SNK_SW
			TI_SNK_B_SE,      //T_SNK_SE
			TI_SNKT_B_S,      //T_SNKT_S
			TI_SNKT_B_W,      //T_SNKT_W
			TI_SNKT_B_N,      //T_SNKT_N
			TI_SNKT_B_E       //T_SNKT_E
		},
		{
			TI_SNK_G_EW,      //T_SNK_EW
			TI_SNK_G_NS,      //T_SNK_NS
			TI_SNK_G_NW,      //T_SNK_NW
			TI_SNK_G_NE,      //T_SNK_NE
			TI_SNK_G_SW,      //T_SNK_SW
			TI_SNK_G_SE,      //T_SNK_SE
			TI_SNKT_G_S,      //T_SNKT_S
			TI_SNKT_G_W,      //T_SNKT_W
			TI_SNKT_G_N,      //T_SNKT_N
			TI_SNKT_G_E       //T_SNKT_E
		}
	};

	UINT wMonsterIndex;
	switch (wType)
	{
		case M_SERPENT: wMonsterIndex = 0;  break;
		case M_SERPENTB: wMonsterIndex = 1;  break;
		case M_SERPENTG: wMonsterIndex = 2;  break;
		case M_ROCKGIANT:
		default: ASSERT(!"Bad serpent type."); wMonsterIndex = 0; break;
	}
	ASSERT(bIsSerpentTile(wTileNo));
	return SerpentPieceImageArray[wMonsterIndex][wTileNo - T_SNK_EW];
}

//*****************************************************************************
UINT GetTileImageForRockGiantPiece(const UINT wTileNo, const UINT wO, const UINT wFrame)
{
	static const UINT RockGiantPieceImageArray[2][3][ORIENTATION_COUNT] =
	{{{
		//Frame 0
			TI_ROCKGIANT_NW1, TI_ROCKGIANT_N1, TI_ROCKGIANT_NE1,
			TI_ROCKGIANT_W1,  DONT_USE,       TI_ROCKGIANT_E1,
			TI_ROCKGIANT_SW1, TI_ROCKGIANT_S1, TI_ROCKGIANT_SE1
		},{
			TI_ROCKGIANT_NW2, TI_ROCKGIANT_N2, TI_ROCKGIANT_NE2,
			TI_ROCKGIANT_W2,  DONT_USE,       TI_ROCKGIANT_E2,
			TI_ROCKGIANT_SW2, TI_ROCKGIANT_S2, TI_ROCKGIANT_SE2
		},{
			TI_ROCKGIANT_NW3, TI_ROCKGIANT_N3, TI_ROCKGIANT_NE3,
			TI_ROCKGIANT_W3,  DONT_USE,       TI_ROCKGIANT_E3,
			TI_ROCKGIANT_SW3, TI_ROCKGIANT_S3, TI_ROCKGIANT_SE3
		}},{{
		//Frame 1
			TI_ROCKGIANTA_NW1, TI_ROCKGIANTA_N1, TI_ROCKGIANTA_NE1,
			TI_ROCKGIANTA_W1,  DONT_USE,       TI_ROCKGIANTA_E1,
			TI_ROCKGIANTA_SW1, TI_ROCKGIANTA_S1, TI_ROCKGIANTA_SE1
		},{
			TI_ROCKGIANTA_NW2, TI_ROCKGIANTA_N2, TI_ROCKGIANTA_NE2,
			TI_ROCKGIANTA_W2,  DONT_USE,       TI_ROCKGIANTA_E2,
			TI_ROCKGIANTA_SW2, TI_ROCKGIANTA_S2, TI_ROCKGIANTA_SE2
		},{
			TI_ROCKGIANTA_NW3, TI_ROCKGIANTA_N3, TI_ROCKGIANTA_NE3,
			TI_ROCKGIANTA_W3,  DONT_USE,       TI_ROCKGIANTA_E3,
			TI_ROCKGIANTA_SW3, TI_ROCKGIANTA_S3, TI_ROCKGIANTA_SE3
	}}};

	ASSERT(wTileNo < 3);
	ASSERT(IsValidOrientation(wO));
	return RockGiantPieceImageArray[wFrame][wTileNo][wO];
}

//*****************************************************************************
UINT GetTileImageForTileNo(
//Gets a tile image to display a tile.  Oversimplified for now.
//
//Params:
	const UINT wTileNo)  //(in)   A T_* constant.
//
//Returns:
//TI_* constant or CALC_NEEDED if tile needs to be calculated based on
//other context.
{
	static const UINT TileImageArray[TILE_COUNT] =
	{
		TI_TEMPTY,        //T_EMPTY
		TI_FLOOR,         //T_FLOOR
		TI_SPIKE,         //T_PIT
		TI_STAIRS,        //T_STAIRS
		CALC_NEEDED,      //T_WALL
		CALC_NEEDED,      //T_WALL_B
		CALC_NEEDED,      //T_DOOR_C
		CALC_NEEDED,      //T_DOOR_G
		CALC_NEEDED,      //T_DOOR_R
		CALC_NEEDED,      //T_DOOR_Y
		CALC_NEEDED,      //T_DOOR_YO
		TI_TRAPDOOR,      //T_TRAPDOOR
		CALC_NEEDED,      //T_OBSTACLE
		TI_ARROW_N,       //T_ARROW_N
		TI_ARROW_NE,      //T_ARROW_NE
		TI_ARROW_E,       //T_ARROW_E
		TI_ARROW_SE,      //T_ARROW_SE
		TI_ARROW_S,       //T_ARROW_S
		TI_ARROW_SW,      //T_ARROW_SW
		TI_ARROW_W,       //T_ARROW_W
		TI_ARROW_NW,      //T_ARROW_NW
		TI_HEALTH_MED,    //T_HEALTH_MED
		TI_HEALTH_BIG,    //T_HEALTH_BIG
		TI_SCROLL,        //T_SCROLL
		CALC_NEEDED,      //T_ORB
		TI_SNK_EW,        //T_SNK_EW  //serpent pieces are really handled in GetTileImageForMonsterTile()
		TI_SNK_NS,        //T_SNK_NS
		TI_SNK_NW,        //T_SNK_NW
		TI_SNK_NE,        //T_SNK_NE
		TI_SNK_SW,        //T_SNK_SW
		TI_SNK_SE,        //T_SNK_SE
		TI_SNKT_S,        //T_SNKT_S
		TI_SNKT_W,        //T_SNKT_W
		TI_SNKT_N,        //T_SNKT_N
		TI_SNKT_E,        //T_SNKT_E
		CALC_NEEDED,      //T_TAR
		TI_CHECKPOINT,    //T_CHECKPOINT
		CALC_NEEDED,      //T_DOOR_B
		TI_UNSPECIFIED,   //T_POTION_SP //unused
		CALC_NEEDED,      //T_BRIAR_SOURCE
		CALC_NEEDED,      //T_BRIAR_DEAD
		CALC_NEEDED,      //T_BRIAR_LIVE
		TI_TEMPTY,        //T_LIGHT_CEILING
		TI_BOMB,          //T_BOMB
		CALC_NEEDED,      //T_FUSE
		TI_NODIAGONAL,    //T_NODIAGONAL
		CALC_NEEDED,      //T_TOKEN
		TI_TUNNEL_N,      //T_TUNNEL_N
		TI_TUNNEL_S,      //T_TUNNEL_S
		TI_MIRROR,        //T_MIRROR
		TI_ATK_UP,        //T_ATK_UP
		TI_DEF_UP,        //T_DEF_UP
		CALC_NEEDED,      //T_PLATFORM_H
		CALC_NEEDED,      //T_PLATFORM_V
		TI_FLOOR_M,       //T_FLOOR_M //floor textures are further handled in CRoomWidget::RenderRoom()
		TI_ROAD,          //T_FLOOR_ROAD
		TI_GRASS,         //T_FLOOR_GRASS
		TI_DIRT,          //T_FLOOR_DIRT
		TI_ALT,           //T_FLOOR_ALT
		TI_MONEYDOOR,     //T_DOOR_MONEY
		CALC_NEEDED,      //T_MUD
		TI_STAIRSUP,      //T_STAIRS_UP
		CALC_NEEDED,      //T_WALL_H
		TI_TUNNEL_E,      //T_TUNNEL_E
		TI_TUNNEL_W,      //T_TUNNEL_W
		TI_FLOOR_IMAGE,   //T_FLOOR_IMAGE
		CALC_NEEDED,      //T_WALL2
		CALC_NEEDED,      //T_WATER
		CALC_NEEDED,      //T_DOOR_GO
		CALC_NEEDED,      //T_DOOR_CO
		CALC_NEEDED,      //T_DOOR_RO
		CALC_NEEDED,      //T_DOOR_BO
		TI_TRAPDOOR2,     //T_TRAPDOOR2
		CALC_NEEDED,      //T_GOO
		TI_LIGHT,         //T_LIGHT
		TI_HOT,           //T_HOT
		CALC_NEEDED,      //T_GEL
		TI_MAP,           //T_MAP
		CALC_NEEDED,      //T_PRESSPLATE
		TI_BRIDGE,        //T_BRIDGE
		TI_BRIDGE_H,      //T_BRIDGE_H
		TI_BRIDGE_V,      //T_BRIDGE_V
		TI_PIT_IMAGE,     //T_PIT_IMAGE
		CALC_NEEDED,      //T_WALL_IMAGE
		TI_TEMPTY,        //T_DARK_CEILING
		TI_WALLLIGHT,     //T_WALLLIGHT
		CALC_NEEDED,      //T_KEY
		CALC_NEEDED,      //T_SWORD
		TI_MONEYDOOR_O,   //T_DOOR_MONEYO
		CALC_NEEDED,      //T_SHIELD
		TI_HEALTH_SM,     //T_HEALTH_SM
		CALC_NEEDED,      //T_ACCESSORY
		TI_MAP_DETAIL,    //T_MAP_DETAIL
		TI_HEALTH_HUGE,   //T_HEALTH_HUGE
		TI_ATK_UP3,       //T_ATK_UP3
		TI_ATK_UP10,      //T_ATK_UP10
		TI_DEF_UP3,       //T_DEF_UP3
		TI_DEF_UP10,      //T_DEF_UP10
		TI_CRATE,         //T_CRATE
		CALC_NEEDED,      //T_PRESSPLATE_BROKEN_VIRTUAL
		TI_SHOVEL_1,      //T_SHOVEL1
		TI_SHOVEL_3,      //T_SHOVEL3
		TI_SHOVEL_10,     //T_SHOVEL10
		TI_DIRT_1,        //T_DIRT1
		TI_DIRT_3,        //T_DIRT3
		TI_DIRT_5,        //T_DIRT5
		CALC_NEEDED,      //T_THINICE
		TI_ARROW_OFF_N,   //T_ARROW_OFF_N
		TI_ARROW_OFF_NE,  //T_ARROW_OFF_NE
		TI_ARROW_OFF_E,   //T_ARROW_OFF_E
		TI_ARROW_OFF_SE,  //T_ARROW_OFF_SE
		TI_ARROW_OFF_S,   //T_ARROW_OFF_S
		TI_ARROW_OFF_SW,  //T_ARROW_OFF_SW
		TI_ARROW_OFF_W,   //T_ARROW_OFF_W
		TI_ARROW_OFF_NW,  //T_ARROW_OFF_NW
		CALC_NEEDED,      //T_MIST
		TI_MISTVENT,      //T_MISTVENT
		TI_FIRETRAP,      //T_FIRETRAP
		TI_FIRETRAP_ON,   //T_FIRETRAP_ON
		TI_POWDER_KEG,    //T_POWDER_KEG
		TI_OVERHEAD_IMAGE,//T_OVERHEAD_IMAGE
	};

	ASSERT(IsValidTileNo(wTileNo));
	return TileImageArray[wTileNo];
}

//*****************************************************************************
UINT GetTileImageForMapIcon(const ScriptVars::MapIcon mapIcon)
{
	ASSERT(mapIcon < ScriptVars::MapIconCount);
	static const UINT MapIconImageArray[ScriptVars::MapIconCount] = {
		TI_MAP_ICON_QUESTION_MARK, //MI_None
		TI_MAP_ICON_SWORD,         //MI_Sword
		TI_MAP_ICON_SHIELD,        //MI_Shield
		TI_MAP_ICON_STAR,          //MI_Star
		TI_MAP_ICON_SKULL,         //MI_Skill
		TI_MAP_ICON_ARROW_NORTH,   //MI_ArrowNorth
		TI_MAP_ICON_ARROW_EAST,    //MI_ArrowEast
		TI_MAP_ICON_ARROW_SOUTH,   //MI_ArrowSouth
		TI_MAP_ICON_ARROW_WEST,    //MI_ArrowWest
		TI_MAP_ICON_STAIR_UP,      //MI_StairsUp
		TI_MAP_ICON_STAIR_DOWN,    //MI_StairsDown
		TI_MAP_ICON_CHEST,         //MI_Chest
		TI_MAP_ICON_GEAR,          //MI_Gear
		TI_MAP_ICON_MONEY_BAG,     //MI_MoneyBag
		TI_MAP_ICON_KEY_BLUE,      //MI_KeyBlue
		TI_MAP_ICON_KEY_WHITE,     //MI_KeyWhite
		TI_MAP_ICON_QUESTION_MARK  //MI_QuestionMark
	};

	return MapIconImageArray[mapIcon];
}

//*****************************************************************************
UINT CalcTileImageFor(
//Calculates a tile image for the given tile at (wCol,wRow).
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wTileNo,
	const UINT wCol, const UINT wRow)   //(in)   Indicates square for which to calculate.
//
//Returns:
//TI_* constant.
{
	//Caller shouldn't have called this routine unless GetTileImageForTileNo()
	//indicated a tile calculation was needed.
	ASSERT(GetTileImageForTileNo(wTileNo) == CALC_NEEDED);

	switch (wTileNo)
	{
		//o-layer
		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
			return CalcTileImageForWall(pRoom, wCol, wRow);
//		case T_STAIRS: return CalcTileImageForStairs(pRoom, wCol, wRow);
//		case T_STAIRS_UP: return CalcTileImageForStairsUp(pRoom, wCol, wRow);
		case T_WALL_H: return CalcTileImageForHiddenWall(pRoom, wCol, wRow);
		case T_WALL_B: return CalcTileImageForBrokenWall(pRoom, wCol, wRow);
		case T_DOOR_Y: case T_DOOR_G: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_PRESSPLATE:
			return CalcTileImageForFourNeighborCC(pRoom, wCol, wRow, wTileNo);
      case T_PLATFORM_W: case T_PLATFORM_P:
			return CalcTileImageForPlatform(pRoom, wCol, wRow, wTileNo);
      case T_GOO:
      case T_WATER:
			case T_THINICE: return CalcTileImageForWater(pRoom, wCol, wRow, wTileNo);

		//t-layer
		case T_OBSTACLE:  return CalcTileImageForObstacle(pRoom, wCol, wRow);
		case T_TAR: case T_MUD: case T_GEL:
			return CalcTileImageForTarstuff(pRoom, wCol, wRow);
		case T_FUSE:   return CalcTileImageForFuse(pRoom, wCol, wRow);
		case T_ORB:
		{
			COrbData *pOrb = pRoom->GetOrbAtCoords(wCol,wRow);
			if (!pOrb) return TI_ORB_D;
			switch (pOrb->eType)
			{
				case OT_NORMAL: return TI_ORB_D;
				case OT_ONEUSE: return TI_ORB_CRACKING;
				case OT_BROKEN: return TI_ORB_CRACKED;
				default: ASSERT(!"Unexpected orb type."); break;
			}
		}
		break;
		case T_TOKEN:
		{
			const UINT tParam = pRoom->GetTParam(wCol,wRow);
			return CalcTileImageForToken(tParam);
		}
		break;
      case T_BRIAR_DEAD: case T_BRIAR_LIVE: case T_BRIAR_SOURCE:
			return CalcTileImageForBriar(pRoom, wCol, wRow, wTileNo);
		case T_KEY:
		{
			const UINT tParam = pRoom->GetTParam(wCol,wRow);
			return CalcTileImageForKey(tParam);
		}
		break;
		case T_SWORD:
		{
			const UINT tParam = pRoom->GetTParam(wCol,wRow);
			return CalcTileImageForSword(tParam);
		}
		break;
		case T_SHIELD:
		{
			const UINT tParam = pRoom->GetTParam(wCol,wRow);
			return CalcTileImageForShield(tParam);
		}
		case T_ACCESSORY:
		{
			const UINT tParam = pRoom->GetTParam(wCol,wRow);
			return CalcTileImageForAccessory(tParam);
		}
		case T_MIST:
			return CalcTileImageForMist(pRoom, wCol, wRow);

		default: break;
	}

	//Shouldn't get here.  Possibly the map in GetTileImageForTileNo() has
	//CALC_NEEDED specified for a tile#, but above switch doesn't handle for
	//this tile#.
	ASSERT(!"Unexpected tile#.");
	return TI_TEMPTY;
}

//*****************************************************************************
UINT CalcTileImageForOSquare(
//Calculates a tile image for a square on o-layer.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Indicates square for which to calculate.
//
//Returns:
//TI_* constant.
{
	return CalcTileImageFor(pRoom, pRoom->GetOSquare(wCol, wRow), wCol, wRow);
}

//*****************************************************************************
UINT CalcTileImageForTSquare(
//Calculates a tile image for a square on t-layer.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Indicates square for which to calculate.
//
//Returns:
//TI_* constant.
{
	return CalcTileImageFor(pRoom, pRoom->GetTSquare(wCol, wRow), wCol, wRow);
}

//*****************************************************************************
UINT CalcTileImageForCoveredTSquare(
//Calculates a tile image for covered square on t-layer.
//
//Params:
	const CDbRoom* pRoom, UINT wCol, UINT wRow)
{
	return CalcTileImageFor(pRoom, pRoom->coveredTSquares.GetAt(wCol, wRow), wCol, wRow);
}

//*****************************************************************************
UINT CalcTileImageForFourNeighborCC(
//Calcs a tile image to display for a four-neighbor connected component type.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow,   //(in)   square.
	const UINT wTileNo)	//(in) object type
//
//Returns:
//TI_* constant.
{
	UINT wCalcCode = 0;

	//If north door, set bit 1.
	if (wRow > 0 && pRoom->GetOSquare(wCol, wRow - 1) == wTileNo)
		wCalcCode += 1;

	//If south door, set bit 2.
	if (wRow < pRoom->wRoomRows - 1 && pRoom->GetOSquare(wCol, wRow + 1) == wTileNo)
		wCalcCode += 2;

	//If west door, set bit 3.
	if (wCol > 0 && pRoom->GetOSquare(wCol - 1, wRow) == wTileNo)
		wCalcCode += 4;

	//If east door, set bit 4.
	if (wCol < pRoom->wRoomCols - 1 && pRoom->GetOSquare(wCol + 1, wRow) == wTileNo)
		wCalcCode += 8;

		// ?.?   0     ?#?      1     ?.?      2     ?#?      3
		// .X.         .X.            .X.            .X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   4     ?#?      5     ?.?      6     ?#?      7
		// #X.         #X.            #X.            #X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   8     ?#?      9     ?.?      10    ?#?      11
		// .X#         .X#            .X#            .X#
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   12    ?#?      13    ? ?      14    ?#?      15
		// #X#         #X#            #X#            #X#
		// ?.?         ?.?            ?#?            ?#?
	static const UINT TYPES = 17;
	static const UINT TileImages[TYPES][16] = {
	{
		TI_DOOR_Y,     TI_DOOR_YN,    TI_DOOR_YS,    TI_DOOR_YNS,
		TI_DOOR_YW,    TI_DOOR_YNW,   TI_DOOR_YSW,   TI_DOOR_YNSW,
		TI_DOOR_YE,    TI_DOOR_YNE,   TI_DOOR_YSE,   TI_DOOR_YNSE,
		TI_DOOR_YWE,   TI_DOOR_YNWE,  TI_DOOR_YSWE,  TI_DOOR_YNSWE
	},{
		TI_DOOR_G,     TI_DOOR_GN,    TI_DOOR_GS,    TI_DOOR_GNS,
		TI_DOOR_GW,    TI_DOOR_GNW,   TI_DOOR_GSW,   TI_DOOR_GNSW,
		TI_DOOR_GE,    TI_DOOR_GNE,   TI_DOOR_GSE,   TI_DOOR_GNSE,
		TI_DOOR_GWE,   TI_DOOR_GNWE,  TI_DOOR_GSWE,  TI_DOOR_GNSWE
	},{
		TI_DOOR_C,     TI_DOOR_CN,    TI_DOOR_CS,    TI_DOOR_CNS,
		TI_DOOR_CW,    TI_DOOR_CNW,   TI_DOOR_CSW,   TI_DOOR_CNSW,
		TI_DOOR_CE,    TI_DOOR_CNE,   TI_DOOR_CSE,   TI_DOOR_CNSE,
		TI_DOOR_CWE,   TI_DOOR_CNWE,  TI_DOOR_CSWE,  TI_DOOR_CNSWE
	},{
		TI_DOOR_R,     TI_DOOR_RN,    TI_DOOR_RS,    TI_DOOR_RNS,
		TI_DOOR_RW,    TI_DOOR_RNW,   TI_DOOR_RSW,   TI_DOOR_RNSW,
		TI_DOOR_RE,    TI_DOOR_RNE,   TI_DOOR_RSE,   TI_DOOR_RNSE,
		TI_DOOR_RWE,   TI_DOOR_RNWE,  TI_DOOR_RSWE,  TI_DOOR_RNSWE
	},{
		TI_DOOR_B,     TI_DOOR_BN,    TI_DOOR_BS,    TI_DOOR_BNS,
		TI_DOOR_BW,    TI_DOOR_BNW,   TI_DOOR_BSW,   TI_DOOR_BNSW,
		TI_DOOR_BE,    TI_DOOR_BNE,   TI_DOOR_BSE,   TI_DOOR_BNSE,
		TI_DOOR_BWE,   TI_DOOR_BNWE,  TI_DOOR_BSWE,  TI_DOOR_BNSWE
	},{
		TI_DOOR_YO,     TI_DOOR_YON,    TI_DOOR_YOS,    TI_DOOR_YONS,
		TI_DOOR_YOW,    TI_DOOR_YONW,   TI_DOOR_YOSW,   TI_DOOR_YONSW,
		TI_DOOR_YOE,    TI_DOOR_YONE,   TI_DOOR_YOSE,   TI_DOOR_YONSE,
		TI_DOOR_YOWE,   TI_DOOR_YONWE,  TI_DOOR_YOSWE,  TI_DOOR_YONSWE
	},{
		TI_DOOR_GO,     TI_DOOR_GON,    TI_DOOR_GOS,    TI_DOOR_GONS,
		TI_DOOR_GOW,    TI_DOOR_GONW,   TI_DOOR_GOSW,   TI_DOOR_GONSW,
		TI_DOOR_GOE,    TI_DOOR_GONE,   TI_DOOR_GOSE,   TI_DOOR_GONSE,
		TI_DOOR_GOWE,   TI_DOOR_GONWE,  TI_DOOR_GOSWE,  TI_DOOR_GONSWE
	},{
		TI_DOOR_CO,     TI_DOOR_CON,    TI_DOOR_COS,    TI_DOOR_CONS,
		TI_DOOR_COW,    TI_DOOR_CONW,   TI_DOOR_COSW,   TI_DOOR_CONSW,
		TI_DOOR_COE,    TI_DOOR_CONE,   TI_DOOR_COSE,   TI_DOOR_CONSE,
		TI_DOOR_COWE,   TI_DOOR_CONWE,  TI_DOOR_COSWE,  TI_DOOR_CONSWE
	},{
		TI_DOOR_RO,     TI_DOOR_RON,    TI_DOOR_ROS,    TI_DOOR_RONS,
		TI_DOOR_ROW,    TI_DOOR_RONW,   TI_DOOR_ROSW,   TI_DOOR_RONSW,
		TI_DOOR_ROE,    TI_DOOR_RONE,   TI_DOOR_ROSE,   TI_DOOR_RONSE,
		TI_DOOR_ROWE,   TI_DOOR_RONWE,  TI_DOOR_ROSWE,  TI_DOOR_RONSWE
	},{
		TI_DOOR_BO,     TI_DOOR_BON,    TI_DOOR_BOS,    TI_DOOR_BONS,
		TI_DOOR_BOW,    TI_DOOR_BONW,   TI_DOOR_BOSW,   TI_DOOR_BONSW,
		TI_DOOR_BOE,    TI_DOOR_BONE,   TI_DOOR_BOSE,   TI_DOOR_BONSE,
		TI_DOOR_BOWE,   TI_DOOR_BONWE,  TI_DOOR_BOSWE,  TI_DOOR_BONSWE
	},{
		TI_PP,      TI_PP_N,    TI_PP_S,    TI_PP_NS,
		TI_PP_W,    TI_PP_NW,   TI_PP_SW,   TI_PP_NSW,
		TI_PP_E,    TI_PP_NE,   TI_PP_SE,   TI_PP_NSE,
		TI_PP_WE,   TI_PP_NWE,  TI_PP_SWE,  TI_PP_NSWE
	},{
		TI_PP_C,     TI_PP_CN,    TI_PP_CS,    TI_PP_CNS,
		TI_PP_CW,    TI_PP_CNW,   TI_PP_CSW,   TI_PP_CNSW,
		TI_PP_CE,    TI_PP_CNE,   TI_PP_CSE,   TI_PP_CNSE,
		TI_PP_CWE,   TI_PP_CNWE,  TI_PP_CSWE,  TI_PP_CNSWE
	},{
		TI_PPT,      TI_PPT_N,    TI_PPT_S,    TI_PPT_NS,
		TI_PPT_W,    TI_PPT_NW,   TI_PPT_SW,   TI_PPT_NSW,
		TI_PPT_E,    TI_PPT_NE,   TI_PPT_SE,   TI_PPT_NSE,
		TI_PPT_WE,   TI_PPT_NWE,  TI_PPT_SWE,  TI_PPT_NSWE
	},{
		TI_PPT_C,     TI_PPT_CN,    TI_PPT_CS,    TI_PPT_CNS,
		TI_PPT_CW,    TI_PPT_CNW,   TI_PPT_CSW,   TI_PPT_CNSW,
		TI_PPT_CE,    TI_PPT_CNE,   TI_PPT_CSE,   TI_PPT_CNSE,
		TI_PPT_CWE,   TI_PPT_CNWE,  TI_PPT_CSWE,  TI_PPT_CNSWE
	},{
		TI_PPB,      TI_PPB_N,    TI_PPB_S,    TI_PPB_NS,
		TI_PPB_W,    TI_PPB_NW,   TI_PPB_SW,   TI_PPB_NSW,
		TI_PPB_E,    TI_PPB_NE,   TI_PPB_SE,   TI_PPB_NSE,
		TI_PPB_WE,   TI_PPB_NWE,  TI_PPB_SWE,  TI_PPB_NSWE
	},{
		TI_PPB_C,     TI_PPB_CN,    TI_PPB_CS,    TI_PPB_CNS,
		TI_PPB_CW,    TI_PPB_CNW,   TI_PPB_CSW,   TI_PPB_CNSW,
		TI_PPB_CE,    TI_PPB_CNE,   TI_PPB_CSE,   TI_PPB_CNSE,
		TI_PPB_CWE,   TI_PPB_CNWE,  TI_PPB_CSWE,  TI_PPB_CNSWE
	},{
		TI_PPB_D,     TI_PPB_DN,    TI_PPB_DS,    TI_PPB_DNS,
		TI_PPB_DW,    TI_PPB_DNW,   TI_PPB_DSW,   TI_PPB_DNSW,
		TI_PPB_DE,    TI_PPB_DNE,   TI_PPB_DSE,   TI_PPB_DNSE,
		TI_PPB_DWE,   TI_PPB_DNWE,  TI_PPB_DSWE,  TI_PPB_DNSWE
	}};

	ASSERT(wCalcCode < 16);

	UINT wItemType;
	switch (wTileNo)
	{
		case T_DOOR_Y: wItemType = 0; break;
		case T_DOOR_G: wItemType = 1; break;
		case T_DOOR_C: wItemType = 2; break;
		case T_DOOR_R: wItemType = 3; break;
		case T_DOOR_B: wItemType = 4; break;
		case T_DOOR_YO: wItemType = 5; break;
		case T_DOOR_GO: wItemType = 6; break;
		case T_DOOR_CO: wItemType = 7; break;
		case T_DOOR_RO: wItemType = 8; break;
		case T_DOOR_BO: wItemType = 9; break;
		case T_PRESSPLATE:
		{
			COrbData *pPlate = pRoom->GetPressurePlateAtCoords(wCol, wRow);
			if (!pPlate)
				wItemType = 10;
			else
			{
				if (pPlate->eType == OT_ONEUSE)
				{
					wItemType = pPlate->bActive ? 16 : 14; break;
				}
				if (pPlate->eType == OT_BROKEN)
				{
					wItemType = 15; break;
				}
				if (pPlate->bActive)
				{
					wItemType = pPlate->eType == OT_TOGGLE ? 13 : 11;
					break;
				}
				wItemType = pPlate->eType == OT_TOGGLE ? 12 : 10;
			}
		}
		break;
		default: wItemType = 0; ASSERT(!"CalcTileImageForFourNeighborCC: Bad item type"); break;
	}

	return TileImages[wItemType][wCalcCode];
}

//*****************************************************************************
UINT CalcTileImageForFuse(
//Calcs a tile image to display for a fuse square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Door square.
//
//Returns:
//TI_* constant.
{
	//If north fuse/bomb, set bit 1.
	UINT wTSquare, wTCovered;
	UINT wCalcCode = 0;

	if (wRow > 0) {
		wTSquare = pRoom->GetTSquare(wCol, wRow - 1);
		wTCovered = pRoom->coveredTSquares.GetAt(wCol, wRow - 1);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
			wCalcCode = 1;
	}

	//If south fuse/bomb, set bit 2.
	if (wRow < pRoom->wRoomRows - 1) {
		wTSquare = pRoom->GetTSquare(wCol, wRow + 1);
		wTCovered = pRoom->coveredTSquares.GetAt(wCol, wRow + 1);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
			wCalcCode += 2;
	}

	//If west fuse/bomb, set bit 3.
	if (wCol > 0) {
		wTSquare = pRoom->GetTSquare(wCol - 1, wRow);
		wTCovered = pRoom->coveredTSquares.GetAt(wCol - 1, wRow);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
			wCalcCode += 4;
	}

	//If east fuse/bomb, set bit 4.
	if (wCol < pRoom->wRoomCols - 1) {
		wTSquare = pRoom->GetTSquare(wCol + 1, wRow);
		wTCovered = pRoom->coveredTSquares.GetAt(wCol + 1, wRow);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
			wCalcCode += 8;
	}

		// ?.?   0     ?#?      1     ?.?      2     ?#?      3
		// .X.         .X.            .X.            .X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   4     ?#?      5     ?.?      6     ?#?      7
		// #X.         #X.            #X.            #X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   8     ?#?      9     ?.?      10    ?#?      11
		// .X#         .X#            .X#            .X#
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   12    ?#?      13    ? ?      14    ?#?      15
		// #X#         #X#            #X#            #X#
		// ?.?         ?.?            ?#?            ?#?
	static const UINT TileImages[2][16] = {
	{
		TI_FUSE,       TI_FUSE_N,     TI_FUSE_S,     TI_FUSE_NS,
		TI_FUSE_W,     TI_FUSE_NW,    TI_FUSE_SW,    TI_FUSE_NSW,
		TI_FUSE_E,     TI_FUSE_NE,    TI_FUSE_SE,    TI_FUSE_NSE,
		TI_FUSE_WE,    TI_FUSE_NWE,   TI_FUSE_SWE,   TI_FUSE_NSWE
	},{
		TI_FUSE2,      TI_FUSE2_N,    TI_FUSE2_S,    TI_FUSE2_NS,
		TI_FUSE2_W,    TI_FUSE2_NW,   TI_FUSE2_SW,   TI_FUSE2_NSW,
		TI_FUSE2_E,    TI_FUSE2_NE,   TI_FUSE2_SE,   TI_FUSE2_NSE,
		TI_FUSE2_WE,   TI_FUSE2_NWE,  TI_FUSE2_SWE,  TI_FUSE2_NSWE
	}};

	ASSERT(wCalcCode < 16);
	//Draw fuse differently when it's not in 
	const UINT wOSquare = pRoom->GetOSquare(wCol, wRow);
	const UINT wFuseType = bShowsShadow(wOSquare) || bIsPit(wOSquare) ? 0 : 1;
	return TileImages[wFuseType][wCalcCode];
}

//*****************************************************************************
UINT CalcTileImageForMist(
//Calcs a tile image to display for a mist square.
//
//Params:
	const CDbRoom* pRoom, //(in) Room to use for calcs--not necessarily the current room
	const UINT wCol, const UINT wRow //Mist square
)
{
	UINT wCalcCode = 0;

	//If north mist, set bit 1
	if (wRow > 0) {
		if (pRoom->IsEitherTSquare(wCol, wRow - 1, T_MIST))
			wCalcCode = 1;
	}

	//If south mist, set bit 2.
	if (wRow < pRoom->wRoomRows - 1) {
		if (pRoom->IsEitherTSquare(wCol, wRow + 1, T_MIST))
			wCalcCode += 2;
	}

	//If west mist, set bit 3.
	if (wCol > 0) {
		if (pRoom->IsEitherTSquare(wCol - 1, wRow, T_MIST))
			wCalcCode += 4;
	}

	//If east mist, set bit 4.
	if (wCol < pRoom->wRoomCols - 1) {
		if (pRoom->IsEitherTSquare(wCol + 1, wRow, T_MIST))
			wCalcCode += 8;
	}

	// ?.?   0     ?#?      1     ?.?      2     ?#?      3
	// .X.         .X.            .X.            .X.
	// ?.?         ?.?            ?#?            ?#?

	// ?.?   4     ?#?      5     ?.?      6     ?#?      7
	// #X.         #X.            #X.            #X.
	// ?.?         ?.?            ?#?            ?#?

	// ?.?   8     ?#?      9     ?.?      10    ?#?      11
	// .X#         .X#            .X#            .X#
	// ?.?         ?.?            ?#?            ?#?

	// ?.?   12    ?#?      13    ? ?      14    ?#?      15
	// #X#         #X#            #X#            #X#
	// ?.?         ?.?            ?#?            ?#?
	static const UINT TileImages[16] = {
		TI_MIST,       TI_MIST_N,     TI_MIST_S,     TI_MIST_NS,
		TI_MIST_W,     TI_MIST_NW,    TI_MIST_SW,    TI_MIST_NSW,
		TI_MIST_E,     TI_MIST_NE,    TI_MIST_SE,    TI_MIST_NSE,
		TI_MIST_WE,    TI_MIST_NWE,   TI_MIST_SWE,   TI_MIST_NSWE
	};

	ASSERT(wCalcCode < 16);
	return TileImages[wCalcCode];
}

//*****************************************************************************
BYTE GetMistCorners(const CDbRoom* pRoom, const UINT wCol, const UINT wRow)
{
	BYTE corners = 0;
	if (pRoom->IsValidColRow(wCol - 1, wRow - 1) && pRoom->IsEitherTSquare(wCol - 1, wRow - 1, T_MIST))
		corners += 1;

	if (pRoom->IsValidColRow(wCol + 1, wRow - 1) && pRoom->IsEitherTSquare(wCol + 1, wRow - 1, T_MIST))
		corners += 2;

	if (pRoom->IsValidColRow(wCol - 1, wRow + 1) && pRoom->IsEitherTSquare(wCol - 1, wRow + 1, T_MIST))
		corners += 4;

	if (pRoom->IsValidColRow(wCol + 1, wRow + 1) && pRoom->IsEitherTSquare(wCol + 1, wRow + 1, T_MIST))
		corners += 8;

	return corners;
}

//*****************************************************************************
UINT CalcTileImageForToken(const BYTE tParam)
//Calculate image for this type of token.
{
	const bool bOn = bTokenActive(tParam);
	switch (calcTokenType(tParam))
	{
		case RotateArrowsCW: return bOn ? TI_TOKEN_CCW : TI_TOKEN_CW;
		case RotateArrowsCCW: return bOn ? TI_TOKEN_CW : TI_TOKEN_CCW;
		case TarTranslucent: return bOn ? TI_TOKEN_TARON : TI_TOKEN_TAROFF;
		case SwitchTarMud: return TI_TOKEN_TARMUD;
		case SwitchTarGel: return TI_TOKEN_TARGEL;
		case SwitchGelMud: return TI_TOKEN_GELMUD;
//		case PowerTarget: return bOn ? TI_TOKEN_PWR_ON : TI_TOKEN_PWR_OFF;
		case SwordDisarm: return bOn ? TI_TOKEN_SWORD : TI_TOKEN_SWORDOFF;
//		case PersistentCitizenMovement: return TI_TOKEN_CITIZEN;
		default: ASSERT(!"Unexpected token type."); return 0;
	}
}

//*****************************************************************************
UINT CalcTileImageForKey(const BYTE tParam)
//Calculate image for this type of key.
{
	switch (tParam)
	{
		case YellowKey: return TI_YELLOWKEY;
		case GreenKey: return TI_GREENKEY;
		case BlueKey: return TI_BLUEKEY;
		case SkeletonKey: return TI_SKELETONKEY;
		default: return TI_UNSPECIFIED;
	}
}

//*****************************************************************************
UINT CalcTileImageForSword(const BYTE tParam)
//Calculate image for this type of sword.
{
	switch (tParam)
	{
		case NoSword: return TI_TEMPTY;

		case WoodenBlade: return TI_SWORD1;
		case ShortSword: return TI_SWORD2;
		case GoblinSword: return TI_SWORD6;
		case LongSword: return TI_SWORD3;
		case HookSword: return TI_SWORD4;
		case ReallyBigSword: return TI_SWORD5;
		case LuckySword: return TI_SWORD7;
		case SerpentSword: return TI_SWORD8;
		case BriarSword: return TI_SWORD9;
		case WeaponSlot: return TI_SWORD10;
		case Dagger: return TI_DAGGER_SWORD;
		case Staff: return TI_STAFF_SWORD_NEUTRAL;
		case Spear: return TI_SPEAR_SWORD_NEUTRAL;

		default: return TI_UNSPECIFIED;
	}
}

//*****************************************************************************
UINT CalcTileImageForShield(const BYTE tParam)
//Calculate image for this type of shield.
{
	switch (tParam)
	{
		case NoShield: return TI_TEMPTY;

		case WoodenShield: return TI_SHIELD1;
		case BronzeShield: return TI_SHIELD2;
		case SteelShield: return TI_SHIELD3;
		case KiteShield: return TI_SHIELD4;
		case OremiteShield: return TI_SHIELD5;
		case ArmorSlot: return TI_SHIELD6;
		case MirrorShield: return TI_SHIELD7;
		case LeatherShield: return TI_SHIELD8;
		case AluminumShield: return TI_SHIELD9;

		default: return TI_UNSPECIFIED;
	}
}

//*****************************************************************************
UINT CalcTileImageForAccessory(const BYTE tParam)
//Calculate image for this type of accessory.
{
	switch (tParam)
	{
		case NoAccessory: return TI_TEMPTY;

		case GrapplingHook: return TI_ACCESSORY1;
		case WaterBoots: return TI_ACCESSORY2;
		case InvisibilityPotion: return TI_ACCESSORY3;
		case SpeedPotion: return TI_ACCESSORY4;
		case HandBomb: return TI_ACCESSORY5;
		case PickAxe: return TI_ACCESSORY6;
		case WarpToken: return TI_ACCESSORY7;
		case PortableOrb: return TI_ACCESSORY8;
		case LuckyGold: return TI_ACCESSORY9;
		case WallWalking: return TI_ACCESSORY10;
		case XPDoubler: return TI_ACCESSORY11;
		case AccessorySlot: return TI_ACCESSORY12;

		default: return TI_UNSPECIFIED;
	}
}

//*****************************************************************************
bool CastsWallShadow(const UINT t) {
	return bIsWall(t) || bIsCrumblyWall(t) || bIsDoor(t) || bIsDiggableBlock(t);
}

//*****************************************************************************
UINT CalcTileImagesForWallShadow(
//Calcs a tile images to mask shadows on the floor.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Floor square.
//
//Returns:
//TI_* constant (TI_UNSPECIFIED indicates no shadow)
{
	ASSERT(pRoom->IsValidColRow(wCol, wRow));

	//Get tiles in N, NW, and W positions that will be used to calculate shadow
	//images cast onto this tile.  When squares to evaluate are out-of-bounds,
	//make a guess of what the out-of-bounds squares might be.
	UINT wNTileNo, wWTileNo, wNWTileNo;
	if (wCol == 0)
	{
		//W and NW are out-of-bounds.

		//If N is also out-of-bounds then I can handle with quick return.
		if (wRow == 0) return TI_UNSPECIFIED;

		//Pretend the out-of-bounds squares are a continuation of center and north
		//squares.
		wNTileNo = pRoom->GetOSquare(wCol, wRow - 1);
		wWTileNo = pRoom->GetOSquare(wCol, wRow);
		wNWTileNo = wNTileNo;
	}
	else if (wRow == 0)
	{
		//N and NW are out-of-bounds.

		//Pretend the out-of-bounds squares are a continuation of center and north
		//squares.
		wNTileNo = pRoom->GetOSquare(wCol, wRow);
		wWTileNo = pRoom->GetOSquare(wCol - 1, wRow);
		wNWTileNo = wWTileNo;
	}
	else
	{
		//All squares to check are in-bounds.
		wNTileNo = pRoom->GetOSquare(wCol, wRow - 1);
		wWTileNo = pRoom->GetOSquare(wCol - 1, wRow);
		wNWTileNo = pRoom->GetOSquare(wCol - 1, wRow - 1);
	}

	//1. Calculate shadow tile image cast by walls.
	const UINT bIsWallN = CastsWallShadow(wNTileNo)?1:0;
	const UINT bIsWallW = CastsWallShadow(wWTileNo)?1:0;
	const UINT bIsWallNW = CastsWallShadow(wNWTileNo)?1:0;

	const UINT wCalcArrayI =
		(bIsWallNW * 1) +
		(bIsWallW * 2) +
		(bIsWallN * 4);
	static const UINT WallCalcArray[] =
	{
			// ..             #.             ..             #.
			// .              .              #              #
			TI_UNSPECIFIED,   TI_SHADO_DNW,  TI_SHADO_DSW,  TI_SHADO_DW,

			// .#          ##             .#                ##
			// .           .              #                 #
			TI_SHADO_DNE,  TI_SHADO_DN,   TI_SHADO_DNESW,   TI_SHADO_DNWI
	};
	ASSERT(wCalcArrayI < sizeof(WallCalcArray) / sizeof(UINT));
	return WallCalcArray[wCalcArrayI];
}

//*****************************************************************************
UINT CalcTileImageForWall(
//Calcs a tile image to display for a wall square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	//If this tile is an inner wall, then it will get drawn over later.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow) == WALL_INNER)
		return TI_WALL_M;

	UINT wCalcCode = 0;

	//If north wall, set bit 1.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow - 1) == WALL_EDGE) wCalcCode = 1;

	//If south wall, set bit 2.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow + 1) == WALL_EDGE) wCalcCode += 2;

	//If west wall, set bit 3.
	if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow) == WALL_EDGE) wCalcCode += 4;

	//If east wall, set bit 4.
	if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow) == WALL_EDGE) wCalcCode += 8;

	static const UINT TileImages[16] = {

		  // ?.? 0        ?#?    1        ?.?    2        ?#?    3
		  // .X.          .X.             .X.             .X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_NSWE,   TI_WALL_SWE,    TI_WALL_NWE,    TI_WALL_WE,

		  // ?.? 4        ?#?      5      ?.?      6      ?#?      7
		  // #X.          #X.             #X.             #X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_NSE,    TI_WALL_SE,     TI_WALL_NE,     TI_WALL_E,

		  // ?.? 8        ?#?    9        ?.?    10       ?#?    11
		  // .X#          .X#             .X#             .X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_NSW,    TI_WALL_SW,     TI_WALL_NW,     TI_WALL_W,
 
		  // ?.? 12       ?#?    13       ? ?    14       ?#?    15
		  // #X#          #X#             #X#             #X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_NS,     TI_WALL_S,      TI_WALL_N,      TI_WALL
	};

	 switch (TileImages[wCalcCode])
	 {
		  case TI_WALL_NW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_NW : TI_WALL_NW4;
		  case TI_WALL_NE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_NE : TI_WALL_NE8;

		  case TI_WALL_SW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_SW : TI_WALL_SW2;

		  case TI_WALL_SE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_SE : TI_WALL_SE1;

		  case TI_WALL_N:
		  {
				static const UINT TIs[4] = {TI_WALL_N, TI_WALL_N8, TI_WALL_N4, TI_WALL_N12};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_S:
		  {
				static const UINT TIs[4] = {TI_WALL_S, TI_WALL_S1, TI_WALL_S2, TI_WALL_S3};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_W:
		  {
				static const UINT TIs[4] = {TI_WALL_W, TI_WALL_W2, TI_WALL_W4, TI_WALL_W6};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_E:
		  {
				static const UINT TIs[4] = {TI_WALL_E, TI_WALL_E1, TI_WALL_E8, TI_WALL_E9};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL:
		  {
				static const UINT TIs[16] = {TI_WALL, TI_WALL_1, TI_WALL_2, TI_WALL_3, TI_WALL_4,
						  TI_WALL_5, TI_WALL_6, TI_WALL_7, TI_WALL_8, TI_WALL_9, TI_WALL_10, TI_WALL_11,
						  TI_WALL_12, TI_WALL_13, TI_WALL_14, TI_WALL_15};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 4;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 8;
				return TIs[wCalcCode];
		  }

		  default: //TI_WALL_NSWE, TI_WALL_SWE, TI_WALL_NWE, TI_WALL_NSE, TI_WALL_NSW, TI_WALL_WE, TI_WALL_NS
		  return TileImages[wCalcCode];
	 }
}

//*****************************************************************************
UINT CalcTileImageForBrokenWall(
//Calcs a tile image to display for a broken wall square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	//If this tile is an inner wall, then it will get drawn over later.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow) == WALL_INNER)
		return TI_WALL_M;

	UINT wCalcCode = 0;

	//If north wall, set bit 1.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow - 1) == WALL_EDGE) wCalcCode = 1;

	//If south wall, set bit 2.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow + 1) == WALL_EDGE) wCalcCode += 2;

	//If west wall, set bit 3.
	if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow) == WALL_EDGE) wCalcCode += 4;

	//If east wall, set bit 4.
	if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow) == WALL_EDGE) wCalcCode += 8;

	static const UINT TileImages[16] = {

		  // ?.? 0        ?#?    1        ?.?    2        ?#?    3
		  // .X.          .X.             .X.             .X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_BNSWE,  TI_WALL_BSWE,   TI_WALL_BNWE,   TI_WALL_BWE,

		  // ?.? 4        ?#?      5      ?.?      6      ?#?      7
		  // #X.          #X.             #X.             #X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_BNSE,   TI_WALL_BSE,    TI_WALL_BNE,    TI_WALL_BE,

		  // ?.? 8        ?#?    9        ?.?    10       ?#?    11
		  // .X#          .X#             .X#             .X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_BNSW,   TI_WALL_BSW,    TI_WALL_BNW,    TI_WALL_BW,

		  // ?.? 12       ?#?    13       ? ?    14       ?#?    15
		  // #X#          #X#             #X#             #X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_BNS,    TI_WALL_BS,     TI_WALL_BN,     TI_WALL_B
	};

	 switch (TileImages[wCalcCode])
	 {
		  case TI_WALL_BNW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_BNW : TI_WALL_BNW4;
		  case TI_WALL_BNE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_BNE : TI_WALL_BNE8;

		  case TI_WALL_BSW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_BSW : TI_WALL_BSW2;

		  case TI_WALL_BSE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_BSE : TI_WALL_BSE1;

		  case TI_WALL_BN:
		  {
				static const UINT TIs[4] = {TI_WALL_BN, TI_WALL_BN8, TI_WALL_BN4, TI_WALL_BN12};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_BS:
		  {
				static const UINT TIs[4] = {TI_WALL_BS, TI_WALL_BS1, TI_WALL_BS2, TI_WALL_BS3};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_BW:
		  {
				static const UINT TIs[4] = {TI_WALL_BW, TI_WALL_BW2, TI_WALL_BW4, TI_WALL_BW6};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_BE:
		  {
				static const UINT TIs[4] = {TI_WALL_BE, TI_WALL_BE1, TI_WALL_BE8, TI_WALL_BE9};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_B:
		  {
				static const UINT TIs[16] = {TI_WALL_B, TI_WALL_B1, TI_WALL_B2, TI_WALL_B3, TI_WALL_B4,
						  TI_WALL_B5, TI_WALL_B6, TI_WALL_B7, TI_WALL_B8, TI_WALL_B9, TI_WALL_B10, TI_WALL_B11,
						  TI_WALL_B12, TI_WALL_B13, TI_WALL_B14, TI_WALL_B15};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 4;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 8;
				return TIs[wCalcCode];
		  }

		  default:    //TI_WALL_BNSWE, TI_WALL_BSWE, TI_WALL_BNWE, TI_WALL_BNSE, TI_WALL_BNSW,
						  //TI_WALL_BWE, TI_WALL_BNS
		  return TileImages[wCalcCode];
	 }
}

//*****************************************************************************
UINT CalcTileImageForHiddenWall(
//Calcs a tile image to display for a hidden wall square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	//If this tile is an inner wall, then it will get drawn over later.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow) == WALL_INNER)
		return TI_WALL_M;

	UINT wCalcCode = 0;

	//If north wall, set bit 1.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow - 1) == WALL_EDGE) wCalcCode = 1;

	//If south wall, set bit 2.
	if (GetWallTypeAtSquare(pRoom, wCol, wRow + 1) == WALL_EDGE) wCalcCode += 2;

	//If west wall, set bit 3.
	if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow) == WALL_EDGE) wCalcCode += 4;

	//If east wall, set bit 4.
	if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow) == WALL_EDGE) wCalcCode += 8;

	static const UINT TileImages[16] = {

		  // ?.? 0        ?#?    1        ?.?    2        ?#?    3
		  // .X.          .X.             .X.             .X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_HNSWE,  TI_WALL_HSWE,   TI_WALL_HNWE,   TI_WALL_HWE,

		  // ?.? 4        ?#?      5      ?.?      6      ?#?      7
		  // #X.          #X.             #X.             #X.
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_HNSE,   TI_WALL_HSE,    TI_WALL_HNE,    TI_WALL_HE,

		  // ?.? 8        ?#?    9        ?.?    10       ?#?    11
		  // .X#          .X#             .X#             .X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_HNSW,   TI_WALL_HSW,    TI_WALL_HNW,    TI_WALL_HW,

		  // ?.? 12       ?#?    13       ? ?    14       ?#?    15
		  // #X#          #X#             #X#             #X#
		  // ?.?          ?.?             ?#?             ?#?
		  TI_WALL_HNS,    TI_WALL_HS,     TI_WALL_HN,     TI_WALL_H
	};

	 switch (TileImages[wCalcCode])
	 {
		  case TI_WALL_HNW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_HNW : TI_WALL_HNW4;
		  case TI_WALL_HNE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) == WALL_EDGE) ?
					 TI_WALL_HNE : TI_WALL_HNE8;

		  case TI_WALL_HSW:
		  return (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_HSW : TI_WALL_HSW2;

		  case TI_WALL_HSE:
		  return (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) == WALL_EDGE) ?
					 TI_WALL_HSE : TI_WALL_HSE1;

		  case TI_WALL_HN:
		  {
				static const UINT TIs[4] = {TI_WALL_HN, TI_WALL_HN8, TI_WALL_HN4, TI_WALL_HN12};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_HS:
		  {
				static const UINT TIs[4] = {TI_WALL_HS, TI_WALL_HS1, TI_WALL_HS2, TI_WALL_HS3};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_HW:
		  {
				static const UINT TIs[4] = {TI_WALL_HW, TI_WALL_HW2, TI_WALL_HW4, TI_WALL_HW6};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_HE:
		  {
				static const UINT TIs[4] = {TI_WALL_HE, TI_WALL_HE1, TI_WALL_HE8, TI_WALL_HE9};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 2;
				return TIs[wCalcCode];
		  }

		  case TI_WALL_H:
		  {
				static const UINT TIs[16] = {TI_WALL_H, TI_WALL_H1, TI_WALL_H2, TI_WALL_H3, TI_WALL_H4,
						  TI_WALL_H5, TI_WALL_H6, TI_WALL_H7, TI_WALL_H8, TI_WALL_H9, TI_WALL_H10, TI_WALL_H11,
						  TI_WALL_H12, TI_WALL_H13, TI_WALL_H14, TI_WALL_H15};
				wCalcCode = 0;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow - 1) != WALL_EDGE) wCalcCode += 1;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow - 1) != WALL_EDGE) wCalcCode += 2;
				if (GetWallTypeAtSquare(pRoom, wCol + 1, wRow + 1) != WALL_EDGE) wCalcCode += 4;
				if (GetWallTypeAtSquare(pRoom, wCol - 1, wRow + 1) != WALL_EDGE) wCalcCode += 8;
				return TIs[wCalcCode];
		  }

		  default:    //TI_WALL_HNSWE, TI_WALL_HSWE, TI_WALL_HNWE, TI_WALL_HNSE, TI_WALL_HNSW,
						  //TI_WALL_HWE, TI_WALL_HNS
		  return TileImages[wCalcCode];
	 }
}

//*****************************************************************************
WALLTYPE GetWallTypeAtSquare(const CDbRoom *pRoom, const int nCol, const int nRow)
{
	//If square doesn't contain a wall, then return wall type indicating this.
	UINT wTile = pRoom->GetOSquareWithGuessing(nCol, nRow);
	if (!(wTile == T_WALL || wTile == T_WALL_H || wTile == T_WALL_B ||
			wTile == T_WALL2 || wTile == T_WALL_IMAGE))
		return WALL_NONE;

	//Use edge walls on room edge for better appearance.
	if (nRow <= -1 || nRow >= (int)pRoom->wRoomRows) return WALL_EDGE;
	if (nCol <= -1 || nCol >= (int)pRoom->wRoomCols) return WALL_EDGE;

	//Some walls are never "inner" type.
   if (wTile == T_WALL_B || wTile == T_WALL2) return WALL_EDGE;

	//Wall must have one direction without a wall to be an edge wall.
	int nX, nY;
	for (nY = nRow-1; nY <= nRow+1; ++nY)
	{
		for (nX = nCol-1; nX <= nCol+1; ++nX)
		{
			if (nX == nCol && nY == nRow) continue;   //handled above
			wTile = pRoom->GetOSquareWithGuessing(nX, nY);
			if (!(wTile == T_WALL || wTile == T_WALL_H || wTile == T_WALL_B ||
					wTile == T_WALL2 || wTile == T_WALL_IMAGE))
            return WALL_EDGE;
		}
	}

	//Wall is inner wall.
	return WALL_INNER;
}

//*****************************************************************************
void CalcStairPosition(
//Find out what position in a staircase this tile is (from top-left corner).
//Value is 1-based.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow,   //(in)   Square for which to calc.
	UINT& wStairsCol, UINT& wStairsRow) //(out)  position
{
	const UINT wStairTile = pRoom->GetOSquare(wCol, wRow);
	ASSERT(wStairTile==T_STAIRS);
	wStairsCol = wStairsRow = 0;
	UINT wPrevTile;

	if (wCol == 0) {
		wStairsCol = 1;
	} else {
		wPrevTile = wStairTile;
		while (wPrevTile==wStairTile)
		{
			++wStairsCol;
			if (wCol - wStairsCol == 0)
				break;
			wPrevTile = pRoom->GetOSquare(wCol-wStairsCol, wRow);
		}
	}

	if (wRow == 0) {
		wStairsRow = 1;
	} else {
		wPrevTile = wStairTile;
		while (wPrevTile==wStairTile)
		{
			++wStairsRow;
			if (wRow - wStairsRow == 0)
				break;
			wPrevTile = pRoom->GetOSquare(wCol, wRow-wStairsRow);
		}
	}
}

//*****************************************************************************
UINT CalcTileImageForStairs(
//Calcs a tile image to display for a downward stairs square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	if (pRoom->weather.wLight) //in darker rooms
		return TI_UNSPECIFIED; //indicates no cast shadow

	if (pRoom->GetOSquare(wCol, wRow) == T_STAIRS_UP)
		return CalcTileImageForStairsUp(pRoom, wCol, wRow);

	UINT wStairsCol, wStairsRow;
	CalcStairPosition(pRoom, wCol, wRow, wStairsCol, wStairsRow);

	//Now we know the position relative to the beginning of the stairs,
	//we can work out which tile to show.
	const UINT wLastShadedTile=(wStairsCol * 4) - 2;

	if (wStairsRow > wLastShadedTile) return TI_STAIR_SH_FULL;  //Full shading.
	if (wStairsRow == wLastShadedTile) return TI_STAIR_SH4;      // A lot of shading.
	if (wStairsRow + 1 == wLastShadedTile) return TI_STAIR_SH3;  // Medium shading.
	if (wStairsRow + 2 == wLastShadedTile) return TI_STAIR_SH2;  // A bit more shading.
	if (wStairsRow + 3 == wLastShadedTile) return TI_STAIR_SH1;  // Minimal shading.
	//Only option remaining at this point is < wLastShadedTile-3.
	return TI_UNSPECIFIED;                                       //No shading.
}

//*****************************************************************************
void CalcStairUpPosition(
//Find out what position in a staircase this tile is (from bottom-left corner).
//Value is 1-based.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow,   //(in)   Square for which to calc.
	UINT& wStairsCol, UINT& wStairsRow) //(out)  position
{
	const UINT wStairTile = pRoom->GetOSquare(wCol, wRow);
	ASSERT(wStairTile==T_STAIRS_UP);
	wStairsCol = wStairsRow = 0;
	UINT wPrevTile;

	if (wCol == 0) {
		wStairsCol = 1;
	} else {
		wPrevTile = wStairTile;
		while (wPrevTile==wStairTile)
		{
			++wStairsCol;
			if (wCol - wStairsCol == 0)
				break;
			wPrevTile = pRoom->GetOSquare(wCol-wStairsCol, wRow);
		}
	}

	if (wRow == pRoom->wRoomRows - 1) {
		wStairsRow = 1;
	} else {
		wPrevTile = wStairTile;
		while (wPrevTile==wStairTile)
		{
			++wStairsRow;
			if (wRow + wStairsRow >= pRoom->wRoomRows - 1)
				break;
			wPrevTile = pRoom->GetOSquare(wCol, wRow+wStairsRow);
		}
	}
}

//*****************************************************************************
UINT CalcTileImageForStairsUp(
//Calcs a tile image to display for an upward stairs square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
/*
	if (pRoom->weather.wLight)
		return TI_STAIRSUP_1; //no cast shadows in darker rooms
*/

	UINT wStairsCol, wStairsRow;
	CalcStairUpPosition(pRoom, wCol, wRow, wStairsCol, wStairsRow);

	if (wStairsCol==1 && bIsWall(pRoom->GetOSquareWithGuessing((int)wCol-1, wRow)))
	{
		if (wStairsRow==1) return TI_STAIR_SH2;            //small shadow
		if (wStairsRow==2) return TI_STAIR_SH1;            //smaller shadow
	}
	return TI_UNSPECIFIED;                                //No shading.
}

//*****************************************************************************
UINT CalcTileImageForTarstuff(
//Calcs a tile image to display for a tar square.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	static const UINT TAR_TYPES = 3;

	const UINT wTSquare = pRoom->GetTSquare(wCol, wRow);
	UINT wIndex = 0;
	switch (wTSquare)
	{
		case T_TAR: wIndex = 0; break;
		case T_MUD: wIndex = 1; break;
		case T_GEL: wIndex = 2; break;
		default: ASSERT(!"CalcTileImageForTarstuff: invalid tar type"); break;
	}
	ASSERT(wIndex < TAR_TYPES);

	static const UINT wTarType[TAR_TYPES] = {T_TAR, T_MUD, T_GEL};
	static const UINT wMotherType[TAR_TYPES] = {M_TARMOTHER, M_MUDMOTHER, M_GELMOTHER};
	enum TarTileTypes {
		TAR_NSEW, TAR_NWSEI, TAR_NESWI,
		TAR_N, TAR_E, TAR_S,	TAR_W,
		TAR_SEI,	TAR_SWI,	TAR_NEI,	TAR_NWI,
		TAR_NW, TAR_NE, TAR_SE, TAR_SW
	};

	static const UINT wTarTiles[3][15] = {
	{
		TI_TAR_NSEW, TI_TAR_NWSEI, TI_TAR_NESWI,
		TI_TAR_N, TI_TAR_E, TI_TAR_S,	TI_TAR_W,
		TI_TAR_SEI,	TI_TAR_SWI,	TI_TAR_NEI,	TI_TAR_NWI,
		TI_TAR_NW, TI_TAR_NE, TI_TAR_SE, TI_TAR_SW
	},{
		TI_MUD_NSEW, TI_MUD_NWSEI, TI_MUD_NESWI,
		TI_MUD_N, TI_MUD_E, TI_MUD_S,	TI_MUD_W,
		TI_MUD_SEI,	TI_MUD_SWI,	TI_MUD_NEI,	TI_MUD_NWI,
		TI_MUD_NW, TI_MUD_NE, TI_MUD_SE, TI_MUD_SW
	},{
		TI_GEL_NSEW, TI_GEL_NWSEI, TI_GEL_NESWI,
		TI_GEL_N, TI_GEL_E, TI_GEL_S,	TI_GEL_W,
		TI_GEL_SEI,	TI_GEL_SWI,	TI_GEL_NEI,	TI_GEL_NWI,
		TI_GEL_NW, TI_GEL_NE, TI_GEL_SE, TI_GEL_SW
	}};
	const UINT *wTable = wTarTiles[wIndex];

	//Use full tar square on an isolated mother eye.
	if (pRoom->IsMonsterOfTypeAt(wMotherType[wIndex], wCol, wRow) &&
			!pRoom->IsTarStableAt(wCol, wRow, wTarType[wIndex])) return wTable[TAR_NSEW];

	//Going to use bit setting to work out what is around our current tile.
	//The bits for the tiles look like this (where C is our tile):
	//
	//   1     2     4
	// 128     C     8
	//  64    32    16
	//
	//Bits will be set in the bNotTarTiles byte if the square in question does
	//not have tar. Ie. if our tile (C) was against the northern wall but had
	//tar all around it apart from that, the bNotTarTiles would be 7 (1 or 2 or 4).
	//from that, we will then score each point of the compass and use the scores to
	//decide which return value to use. The scores will be out of 3 for each point
	//of the compass the lower the score, the more surrounding squares with tar in
	//them.

	//To make everything nice and easy, we're going to have an array storing the
	//bit values of each tile. These constants point to the elements in that array
	//and the compass points in the scores array. The order of this makes the scoring
	//easier later.
	static const UINT I_NWTILE  = 0;
	static const UINT I_NTILE   = 1;
	static const UINT I_NETILE  = 2;
	static const UINT I_ETILE   = 3;
	static const UINT I_SETILE  = 4;
	static const UINT I_STILE   = 5;
	static const UINT I_SWTILE  = 6;
	static const UINT I_WTILE   = 7;
	static const UINT I_CTILE   = 8;

	//Bits to be set for each missing tile. Note the tile order spirals clockwise
	//from the NW.
	static const UINT B_TILEBITS[] =
	{
		1, 2, 4, 8, 16, 32, 64, 128
	};

	//Adjustments needed to wRow and wCol to get tile corresponding to the tile
	//order above.
	static const int I_ROWADJUST[] =
	{
		-1, -1, -1, 0, 1, 1, 1, 0
	};
	static const int I_COLADJUST[] =
	{
		-1, 0, 1, 1, 1, 0, -1, -1
	};

	//Initialise the scores array.
	int iCompassScores[I_CTILE] = {0};//A score for each point of the compass.
	int iTotalScore = 0;    //Total of all compass scores (easier for some decisions).
	UINT bNotTarTiles = 0;  //Byte for bit setting the non tar tiles.
	UINT wCheckRow = 0;        //Row position of the tile to check for tar.
	UINT wCheckCol = 0;        //Column position of the tile to check for tar.

	//Connectivity rule.
	CCoordSet tiles;
	pRoom->GetTarConnectedComponent(wCol, wRow, tiles, true);

	//Find out which of the 8 tiles around us don't have tarstuff in them.
	for (UINT iTilePointer=0; iTilePointer < I_CTILE; ++iTilePointer)
	{
		//Find out what is in this particular surrounding tile.
		wCheckCol = wCol + I_COLADJUST[iTilePointer];
		wCheckRow = wRow + I_ROWADJUST[iTilePointer];
		const bool bSameTileType = pRoom->IsValidColRow(wCheckCol, wCheckRow) ?
			pRoom->GetTSquare(wCheckCol, wCheckRow) == wTSquare : false;

		//If the surrounding tile doesn't have tar in it then set the relevant
		//bit in bNotTarTiles and work out the scores for each point of the compass. If a
		//tile surrounding ours is empty, it and the ones next to it in our tile order
		//will get +1 score so basically, if 2 tiles are empty of tar, they will both get
		//a score of 2. If 3 tiles together are empty (i.e. around a corner) the middle
		//tile will score 3 and the other 2 will score 2.
		if (!bSameTileType ||
				!tiles.has(wCheckCol, wCheckRow)) //must belong to the same connected component
		{
			bNotTarTiles = bNotTarTiles | B_TILEBITS[iTilePointer];
			int iPrevTile = iTilePointer - 1;
			int iNextTile = iTilePointer + 1;

			if (iPrevTile < (int)I_NWTILE) iPrevTile = I_WTILE;
			if (iNextTile==(int)I_CTILE) iNextTile = I_NWTILE;

			++iCompassScores[iTilePointer];
			++iCompassScores[iPrevTile];
			++iCompassScores[iNextTile];
			iTotalScore += 3;
		}
	}

	//Now we can work out what to return.
	if (iTotalScore==0) return wTable[TAR_NSEW];      //Surrounded by tar.
	if (iTotalScore>15) return wTable[TAR_NWSEI];  //Not much Tar around this tile so just use any.

	//Only 1 empty square.
	if (iTotalScore==3)
	{
		if (bNotTarTiles & B_TILEBITS[I_NTILE]) return wTable[TAR_N];
		if (bNotTarTiles & B_TILEBITS[I_ETILE]) return wTable[TAR_E];
		if (bNotTarTiles & B_TILEBITS[I_STILE]) return wTable[TAR_S];
		if (bNotTarTiles & B_TILEBITS[I_WTILE]) return wTable[TAR_W];
		if (bNotTarTiles & B_TILEBITS[I_NWTILE]) return wTable[TAR_SEI];
		if (bNotTarTiles & B_TILEBITS[I_NETILE]) return wTable[TAR_SWI];
		if (bNotTarTiles & B_TILEBITS[I_SWTILE]) return wTable[TAR_NEI];
		if (bNotTarTiles & B_TILEBITS[I_SETILE]) return wTable[TAR_NWI];
	}

	//Normal Corners.
	if (iCompassScores[I_NWTILE]==3) return wTable[TAR_NW];
	if (iCompassScores[I_NETILE]==3) return wTable[TAR_NE];
	if (iCompassScores[I_SETILE]==3) return wTable[TAR_SE];
	if (iCompassScores[I_SWTILE]==3) return wTable[TAR_SW];

	//Complex Corners.
	if (bNotTarTiles & B_TILEBITS[I_NTILE])
	{
		if ((bNotTarTiles & B_TILEBITS[I_WTILE]) || (bNotTarTiles & B_TILEBITS[I_SWTILE]))
			return wTable[TAR_NW];
		if ((bNotTarTiles & B_TILEBITS[I_ETILE]) || (bNotTarTiles & B_TILEBITS[I_SETILE]))
			return wTable[TAR_NE];
	}
	if ((bNotTarTiles & B_TILEBITS[I_NETILE]) && (bNotTarTiles & B_TILEBITS[I_WTILE]))
		return wTable[TAR_NW];
	if ((bNotTarTiles & B_TILEBITS[I_NWTILE]) && (bNotTarTiles & B_TILEBITS[I_ETILE]))
		return wTable[TAR_NE];

	if (bNotTarTiles & B_TILEBITS[I_STILE])
	{
		if ((bNotTarTiles & B_TILEBITS[I_WTILE]) || (bNotTarTiles & B_TILEBITS[I_NWTILE]))
			return wTable[TAR_SW];
		if ((bNotTarTiles & B_TILEBITS[I_ETILE]) || (bNotTarTiles & B_TILEBITS[I_NETILE]))
			return wTable[TAR_SE];
	}
	if ((bNotTarTiles & B_TILEBITS[I_SETILE]) && (bNotTarTiles & B_TILEBITS[I_WTILE]))
		return wTable[TAR_SW];
	if ((bNotTarTiles & B_TILEBITS[I_SWTILE]) && (bNotTarTiles & B_TILEBITS[I_ETILE]))
		return wTable[TAR_SE];

	//Edges.
	if (iCompassScores[I_NTILE]>1)
	{
		//Check for special case:
		//   M     M     M    T: Tar
		//  TTT   MTT   TTM   M: Mother on tar
		//  TTT    TT   TT
		return wTable[(bNotTarTiles & (B_TILEBITS[I_NWTILE] | B_TILEBITS[I_NTILE] | B_TILEBITS[I_NETILE]))
				== (B_TILEBITS[I_NWTILE] | B_TILEBITS[I_NETILE]) ? TAR_NSEW : TAR_N];
	}
	if (iCompassScores[I_ETILE]>1)
	{
		//Check for special case:
		//  TT    TT    T: Tar
		//  TTM   TTM   M: Mother on tar
		//  TT     M
		return wTable[(bNotTarTiles & (B_TILEBITS[I_NETILE] | B_TILEBITS[I_ETILE] | B_TILEBITS[I_SETILE]))
				== (B_TILEBITS[I_NETILE] | B_TILEBITS[I_SETILE]) ? TAR_NSEW : TAR_E];
	}
	if (iCompassScores[I_STILE]>1)
	{
		//Check for special case:
		//  TTT    TT   T: Tar
		//  TTT   MTT   M: Mother on tar
		//   M     M
		return wTable[(bNotTarTiles & (B_TILEBITS[I_SWTILE] | B_TILEBITS[I_STILE] | B_TILEBITS[I_SETILE]))
				== (B_TILEBITS[I_SWTILE] | B_TILEBITS[I_SETILE]) ? TAR_NSEW : TAR_S];
	}
	if (iCompassScores[I_WTILE]>1)
	{
		//Check for special case:
		//   TT   T: Tar
		//  MTT   M: Mother on tar
		//   TT
		return wTable[(bNotTarTiles & (B_TILEBITS[I_NWTILE] | B_TILEBITS[I_WTILE] | B_TILEBITS[I_SWTILE]))
				== (B_TILEBITS[I_NWTILE] | B_TILEBITS[I_SWTILE]) ? TAR_NSEW : TAR_W];
	}

	//Inner corners.
	if (iCompassScores[I_NWTILE]==1)
	{
		if (iCompassScores[I_SETILE]==1) return wTable[TAR_NWSEI];
		return wTable[TAR_SEI];
	}

	if (iCompassScores[I_NETILE]==1)
	{
		if (iCompassScores[I_SWTILE]==1) return wTable[TAR_NESWI];
		return wTable[TAR_SWI];
	}

	if (iCompassScores[I_SETILE]==1) return wTable[TAR_NWI];
	if (iCompassScores[I_SWTILE]==1) return wTable[TAR_NEI];

	//We shouldn't get to here but if we do, we have a weird number of tiles
	//with tar in around this one so return an arbitrary tile (hopefully, it
	//will look nice enough.
	return wTable[TAR_NESWI];
}

//*****************************************************************************
void GetObstacleStats(
//Returns the type-size index of the obstacle at (col,row) and the relative position of this
//tile in the obstacle.
	const CDbRoom *pRoom,   //(in) Room to use for calcs--not necessarily the
	                        //     current room.
	const UINT wCol, const UINT wRow, //(in)
	UINT& wObSizeIndex, UINT& xPos, UINT& yPos)  //(out)
{
	const UINT obType = calcObstacleType(pRoom->GetTParam(wCol, wRow));
	ASSERT(obType);

	//Find obstacle edges to figure out which relative x/y position this tile is at.
	xPos = yPos = 0;
	UINT x = wCol, y = wRow;
	while (x > 0) {
		if (bObstacleLeft(pRoom->GetTParam(x, wRow))) break; //found left edge
		if (pRoom->GetTSquare(x - 1, wRow) != T_OBSTACLE) break;
		if (calcObstacleType(pRoom->GetTParam(x - 1, wRow)) != obType) break;
	   ++xPos;
		--x;
	}
	if (xPos >= MAX_OBSTACLE_SIZE)
	{
		//Invalid position.
		ASSERT(!"Invalid obstacle x position");
		xPos = 0;
		x = 1;
	} else {
		x = wCol+1;
		while (x < pRoom->wRoomCols) {
			if (bObstacleLeft(pRoom->GetTParam(x, wRow))) break;
			if (pRoom->GetTSquare(x, wRow) != T_OBSTACLE) break;
			if (calcObstacleType(pRoom->GetTParam(x, wRow)) != obType) break;
			++x;
		}
		x -= wCol - xPos; //x dimension of this obstacle
		if (x > MAX_OBSTACLE_SIZE)
			x = MAX_OBSTACLE_SIZE; //can't draw this obstacle larger than this
	}
	ASSERT(xPos < x);

	while (y > 0) {
		if (bObstacleTop(pRoom->GetTParam(wCol, y))) break;  //found top edge
		if (pRoom->GetTSquare(wCol, y - 1) != T_OBSTACLE) break;
		if (calcObstacleType(pRoom->GetTParam(wCol, y - 1)) != obType) break;
		++yPos;
		--y;
	}
	if (yPos >= MAX_OBSTACLE_SIZE)
	{
		//Invalid position.
		ASSERT(!"Invalid obstacle y position");
		yPos = 0;
		y = 1;
	} else {
		y = wRow+1;
		while (y < pRoom->wRoomRows) {
			if (bObstacleTop(pRoom->GetTParam(wCol, y))) break;
			if (pRoom->GetTSquare(wCol, y) != T_OBSTACLE) break;
			if (calcObstacleType(pRoom->GetTParam(wCol, y)) != obType) break;
			++y;
		}
		y -= wRow - yPos; //y dimension of this obstacle
		if (y > MAX_OBSTACLE_SIZE)
			y = MAX_OBSTACLE_SIZE; //can't draw this obstacle larger than this
	}
	ASSERT(yPos < y);

	for (UINT wI=0; wI<MAX_OBSTACLE_SIZE; ++wI)
	{
		const UINT wObIndex = obstacleIndices[obType][wI];
		if (!wObIndex)
			break; //no more sizes to check for this type

		const UINT wXDim = obstacleDimensions[wObIndex][0];
		const UINT wYDim = obstacleDimensions[wObIndex][1];
		if (x == wXDim && y == wYDim)
		{
			//Found a size match for obstacles of this type
			wObSizeIndex = wI;
			return;
		}
	}
	//Size match was not found.  Use the smallest size for this type.
//	ASSERT(!"Invalid obstacle size."); //could happen if rotating a non-square shape
	wObSizeIndex = xPos = yPos = 0;
}

//*****************************************************************************
UINT CalcTileImageForObstacle(
//Calcs a tile image to display for an obstacle.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow)   //(in)   Square for which to calc.
//
//Returns:
//TI_* constant.
{
	//Get obstacle type.
	const UINT obType = calcObstacleType(pRoom->GetTParam(wCol, wRow));
	ASSERT(obType);
	ASSERT(obType < MAX_OBSTACLE_TYPES);

	//Get index of size variant of this obstacle type.
	UINT wObTypeSizeIndex, wXPos, wYPos;
	GetObstacleStats(pRoom, wCol, wRow, wObTypeSizeIndex, wXPos, wYPos);

	const UINT obIndex = obstacleIndices[obType][wObTypeSizeIndex];
	ASSERT(obIndex);

	ASSERT(obstacleTile[obIndex][wXPos][wYPos]);
	return obstacleTile[obIndex][wXPos][wYPos];
}

//*****************************************************************************
inline bool bDrawPit(const CDbRoom *pRoom, const UINT x, const UINT y)
{
	const UINT wOSquare = pRoom->GetOSquare(x,y);
	return bIsPit(wOSquare) || wOSquare==T_TRAPDOOR || wOSquare==T_PLATFORM_P ||
			bIsBridge(wOSquare);
}

void CalcTileCoordForPit(
//Calcs a mosaic (x,y) coord to display for a pit edge.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
									//    current room.
	const UINT wCol, const UINT wRow,   //(in)   Square for which to calc.
	EDGES* edges)        //(out)  position on pit side; width of this pit side to the right
{
	ASSERT(pRoom->IsValidColRow(wCol, wRow));
	ASSERT(bDrawPit(pRoom,wCol,wRow));  //there should be pit in the queried square

	//Look north until I find a non-pit square.
	UINT wLookRow = wRow;
	while (wLookRow != (UINT)(-1) && bDrawPit(pRoom,wCol, wLookRow)) --wLookRow;
	edges->wPitY = wLookRow == (UINT)(-1) ? wLookRow : wRow - wLookRow - 1;

	//Look west to find where pit edge starts.
	UINT wLookCol = wCol;
	while (wLookCol != (UINT)(-1) &&
			(wLookRow == (UINT)(-1) || !bDrawPit(pRoom, wLookCol, wLookRow)) &&
			bDrawPit(pRoom, wLookCol, wLookRow+1))
		--wLookCol;
	edges->wPitX = wCol - wLookCol - 1;

	//Look east to see how much further pit edge extends.
	wLookCol = wCol;
	while (wLookCol < pRoom->wRoomCols &&
			(wLookRow == (UINT)(-1) || !bDrawPit(pRoom, wLookCol, wLookRow)) &&
			bDrawPit(pRoom, wLookCol, wLookRow+1))
		++wLookCol;
	edges->wPitRemaining = wLookCol - wCol;
}

//*****************************************************************************
UINT CalcTileImageForPlatform(
//Calcs a tile image to display for a platform tile.
//
//Params:
   const CDbRoom *pRoom,   //(in)  Room to use for calcs--not necessarily the
                                           //              current room.
   const UINT wCol, const UINT wRow,       //(in)  Platform tile.
	const UINT wTileNo)  //(in)
//
//Returns:
//TI_* constant.
{
   //Separate adjacent platforms only need be distinguished during play.
   bool bInGamePlatform = pRoom->GetCurrentGame() != NULL;
   CPlatform *pPlatform = bInGamePlatform ? pRoom->GetPlatformAt(wCol, wRow) : NULL;
   if (!pPlatform) bInGamePlatform = false;

   UINT wCalcCode = 0;

   //If north platform, set bit 1.
	if (wRow > 0 && pRoom->GetOSquare(wCol, wRow - 1) == wTileNo &&
	      (!bInGamePlatform || pPlatform->IsAt(wCol, wRow - 1)))
		++wCalcCode;

   //If south platform, set bit 2.
   if (wRow < pRoom->wRoomRows - 1 &&
         pRoom->GetOSquare(wCol, wRow + 1) == wTileNo &&
         (!bInGamePlatform || pPlatform->IsAt(wCol, wRow + 1)))
      wCalcCode += 2;

   //If west platform, set bit 3.
   if (wCol > 0 &&
         pRoom->GetOSquare(wCol - 1, wRow) == wTileNo &&
         (!bInGamePlatform || pPlatform->IsAt(wCol - 1, wRow)))
      wCalcCode += 4;

   //If east platform, set bit 4.
   if (wCol < pRoom->wRoomCols - 1 &&
         pRoom->GetOSquare(wCol + 1, wRow) == wTileNo &&
         (!bInGamePlatform || pPlatform->IsAt(wCol + 1, wRow)))
      wCalcCode += 8;

		// ?.?  0            ?#?             1       ?.?             2       ?#?                3
		// .X.               .X.                     .X.                     .X.
		// ?.?               ?.?                     ?#?                     ?#?

		// ?.?  4            ?#?             5       ?.?             6       ?#?                7
		// #X.               #X.                     #X.                     #X.
		// ?.?               ?.?                     ?#?                     ?#?

		// ?.?  8            ?#?             9       ?.?             10      ?#?                11
		// .X#               .X#                     .X#                     .X#
		// ?.?               ?.?                     ?#?                     ?#?

		// ?.?  12           ?#?             13      ? ?             14      ?#?                15
		// #X#               #X#                     #X#                     #X#
		// ?.?               ?.?                     ?#?                     ?#?
   static const UINT TileImages[2][16] = {
	{
		TI_PLATFORM_W,       TI_PLATFORM_W_N,        TI_PLATFORM_W_S,        TI_PLATFORM_W_NS,
		TI_PLATFORM_W_W,     TI_PLATFORM_W_NW,       TI_PLATFORM_W_SW,       TI_PLATFORM_W_NSW,
		TI_PLATFORM_W_E,     TI_PLATFORM_W_NE,       TI_PLATFORM_W_SE,       TI_PLATFORM_W_NSE,
		TI_PLATFORM_W_WE,    TI_PLATFORM_W_NWE,      TI_PLATFORM_W_SWE,      TI_PLATFORM_W_NSWE
	},{
		TI_PLATFORM_P,       TI_PLATFORM_P_N,        TI_PLATFORM_P_S,        TI_PLATFORM_P_NS,
		TI_PLATFORM_P_W,     TI_PLATFORM_P_NW,       TI_PLATFORM_P_SW,       TI_PLATFORM_P_NSW,
		TI_PLATFORM_P_E,     TI_PLATFORM_P_NE,       TI_PLATFORM_P_SE,       TI_PLATFORM_P_NSE,
		TI_PLATFORM_P_WE,    TI_PLATFORM_P_NWE,      TI_PLATFORM_P_SWE,      TI_PLATFORM_P_NSWE
   }};

   ASSERT(wCalcCode < 16);
   return TileImages[wTileNo == T_PLATFORM_W ? 0 : 1][wCalcCode];
}

//*****************************************************************************
UINT CalcTileImageForBriar(
//Calcs a tile image to display for a briar tile.
//
//Params:
   const CDbRoom *pRoom,   //(in)  Room to use for calcs--not necessarily the
                                           //              current room.
   const UINT wCol, const UINT wRow,       //(in)  Briar tile.
	const UINT wTileNo)  //(in)
//
//Returns:
//TI_* constant.
{
   //Separate adjacent briars only need be distinguished during play.
//   const UINT wBriarIndex = pRoom->briars.getIndexAt(wCol, wRow);
//	const bool bIsSource = pRoom->GetTSquare(wCol, wRow) == T_BRIAR_SOURCE;

   //If north has briar, set bit 1.
	UINT wOTile, wTTile, wCalcCode = 0;
	if (wRow == 0)
		++wCalcCode;
	else {
		wOTile = pRoom->GetOSquare(wCol, wRow - 1);
		wTTile = pRoom->GetTSquare(wCol, wRow - 1);
		if (bIsBriar(wTTile))
//		if (wTTile == T_BRIAR_SOURCE ||  //always connect to adjacent sources
//			 (bIsBriar(wTTile) && (bIsSource || pRoom->briars.getIndexAt(wCol, wRow - 1) == wBriarIndex)))
			++wCalcCode;
	}

   //If south has briar, set bit 2.
   if (wRow+1 >= pRoom->wRoomRows)
		wCalcCode += 2;
	else {
		wOTile = pRoom->GetOSquare(wCol, wRow + 1);
		wTTile = pRoom->GetTSquare(wCol, wRow + 1);
		if (bIsBriar(wTTile))
//		if (wTTile == T_BRIAR_SOURCE ||
//			 (bIsBriar(wTTile) && (bIsSource || pRoom->briars.getIndexAt(wCol, wRow + 1) == wBriarIndex)))
      wCalcCode += 2;
	}

   //If west has briar, set bit 3.
   if (wCol == 0)
		wCalcCode += 4;
	else {
		wOTile = pRoom->GetOSquare(wCol - 1, wRow);
		wTTile = pRoom->GetTSquare(wCol - 1, wRow);
		if (bIsBriar(wTTile))
//		if (wTTile == T_BRIAR_SOURCE ||
//			 (bIsBriar(wTTile) && (bIsSource || pRoom->briars.getIndexAt(wCol - 1, wRow) == wBriarIndex)))
      wCalcCode += 4;
	}

   //If east has briar, set bit 4.
   if (wCol+1 >= pRoom->wRoomCols)
      wCalcCode += 8;
	else {
		wOTile = pRoom->GetOSquare(wCol + 1, wRow);
		wTTile = pRoom->GetTSquare(wCol + 1, wRow);
		if (bIsBriar(wTTile))
//		if (wTTile == T_BRIAR_SOURCE ||
//			 (bIsBriar(wTTile) && (bIsSource || pRoom->briars.getIndexAt(wCol + 1, wRow) == wBriarIndex)))
      wCalcCode += 8;
	}
   ASSERT(wCalcCode < 16);

	// ?.?  0          ?#?           1       ?.?           2       ?#?            3
	// .X.             .X.                   .X.                   .X.
	// ?.?             ?.?                   ?#?                   ?#?

	// ?.?  4          ?#?           5       ?.?           6       ?#?            7
	// #X.             #X.                   #X.                   #X.
	// ?.?             ?.?                   ?#?                   ?#?

	// ?.?  8          ?#?           9       ?.?           10      ?#?            11
	// .X#             .X#                   .X#                   .X#
	// ?.?             ?.?                   ?#?                   ?#?

	// ?.?  12         ?#?           13      ? ?           14      ?#?            15
	// #X#             #X#                   #X#                   #X#
	// ?.?             ?.?                   ?#?                   ?#?
   static const UINT TileImages[5][16] = {
	{
		TI_BRIAREDGE,       TI_BRIAREDGE_N,        TI_BRIAREDGE_S,        TI_BRIAREDGE_NS,
		TI_BRIAREDGE_W,     TI_BRIAREDGE_NW,       TI_BRIAREDGE_SW,       TI_BRIAREDGE_NSW,
		TI_BRIAREDGE_E,     TI_BRIAREDGE_NE,       TI_BRIAREDGE_SE,       TI_BRIAREDGE_NSE,
		TI_BRIAREDGE_WE,    TI_BRIAREDGE_NWE,      TI_BRIAREDGE_SWE,      TI_BRIAREDGE_NSWE
	},{
		TI_BRIAREDGE2,       TI_BRIAREDGE_N2,        TI_BRIAREDGE_S2,        TI_BRIAREDGE_NS2,
		TI_BRIAREDGE_W2,     TI_BRIAREDGE_NW2,       TI_BRIAREDGE_SW2,       TI_BRIAREDGE_NSW2,
		TI_BRIAREDGE_E2,     TI_BRIAREDGE_NE2,       TI_BRIAREDGE_SE2,       TI_BRIAREDGE_NSE2,
		TI_BRIAREDGE_WE2,    TI_BRIAREDGE_NWE2,      TI_BRIAREDGE_SWE2,      TI_BRIAREDGE_NSWE2
	},{
		TI_BRIAR,       TI_BRIAR_N,        TI_BRIAR_S,        TI_BRIAR_NS,
		TI_BRIAR_W,     TI_BRIAR_NW,       TI_BRIAR_SW,       TI_BRIAR_NSW,
		TI_BRIAR_E,     TI_BRIAR_NE,       TI_BRIAR_SE,       TI_BRIAR_NSE,
		TI_BRIAR_WE,    TI_BRIAR_NWE,      TI_BRIAR_SWE,      TI_BRIAR_NSWE
	},{
		TI_BRIAR2,       TI_BRIAR_N2,        TI_BRIAR_S2,        TI_BRIAR_NS2,
		TI_BRIAR_W2,     TI_BRIAR_NW2,       TI_BRIAR_SW2,       TI_BRIAR_NSW2,
		TI_BRIAR_E2,     TI_BRIAR_NE2,       TI_BRIAR_SE2,       TI_BRIAR_NSE2,
		TI_BRIAR_WE2,    TI_BRIAR_NWE2,      TI_BRIAR_SWE2,      TI_BRIAR_NSWE2
	},{
		TI_BRIARROOT,       TI_BRIARROOT_N,        TI_BRIARROOT_S,        TI_BRIARROOT_NS,
		TI_BRIARROOT_W,     TI_BRIARROOT_NW,       TI_BRIARROOT_SW,       TI_BRIARROOT_NSW,
		TI_BRIARROOT_E,     TI_BRIARROOT_NE,       TI_BRIARROOT_SE,       TI_BRIARROOT_NSE,
		TI_BRIARROOT_WE,    TI_BRIARROOT_NWE,      TI_BRIARROOT_SWE,      TI_BRIARROOT_NSWE
	}};

	UINT wTypeIndex;
	switch (wTileNo)
	{
		case T_BRIAR_SOURCE: wTypeIndex = 4; break;
		default:
		case T_BRIAR_LIVE: wTypeIndex = 0 + pseudoRandomTileFrame(pRoom, wCol, wRow, 2); break;
		case T_BRIAR_DEAD:
		{
			//Inner briar connected on all four sides has a 3rd and 4th frame.
			const UINT wFrame = pseudoRandomTileFrame(pRoom, wCol, wRow, wCalcCode == 15 ? 4 : 2); 
			switch (wFrame)
			{
				default: wTypeIndex = 2 + wFrame; break;
				case 2: return TI_BRIAR_NSWE3;
				case 3: return TI_BRIAR_NSWE4;
			}
		}
		break;
	}

   return TileImages[wTypeIndex][wCalcCode];
}

//*****************************************************************************
inline bool bDrawWater(const CDbRoom *pRoom, const UINT x, const UINT y, const UINT wTileNo)
//Returns: true if water should briar up against this tile
{
	UINT wOSquare = pRoom->GetOSquareWithGuessing(x,y);

	//Thin Ice is drawn on top of water, so use the underlying square
	if (wOSquare == T_THINICE) wOSquare = T_WATER;

	if (wOSquare == wTileNo)
		return true;

	if (wTileNo == T_WATER && (wOSquare == T_TRAPDOOR2 || wOSquare == T_PLATFORM_W))
		return true;
	if (wTileNo == T_GOO && (bIsWall(wOSquare) || wOSquare == T_WALL_H))
		return true;

	return false;
}

UINT CalcTileImageForWater(
//Calcs a tile image to display for a body of liquid.
//
//Params:
	const CDbRoom *pRoom,   //(in)   Room to use for calcs--not necessarily the
							//    current room.
	const UINT wCol, const UINT wRow,   //(in)   square.
	const UINT wTileNo)
//
//Returns:
//TI_* constant.
{
#define IsThinIceTile(wTile) ((wTile) == T_THINICE || bIsBridge((wTile)))
	UINT wTileType = 0;
	switch (wTileNo)
	{
		case T_WATER: wTileType = 0; break;
		case T_GOO: wTileType = 1; break;
		case T_THINICE: wTileType = 2; break;
		default: ASSERT(!"CalcTileImageForWater: Wrong tile type"); break;
	}

	UINT wCalcCode = 0;
	if (wTileType == 2) {
		//Thin Ice checks for Thin Ice or bridges
			//If thin ice is north, set bit 1.
		UINT wOSquare = pRoom->GetOSquareWithGuessing(wCol, wRow - 1);
		if (IsThinIceTile(wOSquare))
			++wCalcCode;

		//If thin ice is south, set bit 2.
		wOSquare = pRoom->GetOSquareWithGuessing(wCol, wRow + 1);
		if (IsThinIceTile(wOSquare))
			wCalcCode += 2;

		//If thin ice is west, set bit 3.
		wOSquare = pRoom->GetOSquareWithGuessing(wCol - 1, wRow);
		if (IsThinIceTile(wOSquare))
			wCalcCode += 4;

		//If thin ice is east, set bit 4.
		wOSquare = pRoom->GetOSquareWithGuessing(wCol + 1, wRow);
		if (IsThinIceTile(wOSquare))
			wCalcCode += 8;
	} else {
		//If water to north, set bit 1.
		if (bDrawWater(pRoom, wCol, wRow - 1, wTileNo))
			++wCalcCode;

		//If water to south, set bit 2.
		if (bDrawWater(pRoom, wCol, wRow + 1, wTileNo))
			wCalcCode += 2;

		//If water to west, set bit 3.
		if (bDrawWater(pRoom, wCol - 1, wRow, wTileNo))
			wCalcCode += 4;

		//If water to east, set bit 4.
		if (bDrawWater(pRoom, wCol + 1, wRow, wTileNo))
			wCalcCode += 8;
	}

		// ?.?   0     ?#?      1     ?.?      2     ?#?      3
		// .X.         .X.            .X.            .X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   4     ?#?      5     ?.?      6     ?#?      7
		// #X.         #X.            #X.            #X.
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   8     ?#?      9     ?.?      10    ?#?      11
		// .X#         .X#            .X#            .X#
		// ?.?         ?.?            ?#?            ?#?

		// ?.?   12    ?#?      13    ? ?      14    ?#?      15
		// #X#         #X#            #X#            #X#
		// ?.?         ?.?            ?#?            ?#?
	static const UINT TileImages[3][16] = {
	{
		//Note these are to show the shape of the bank, not the water.
		TI_WATER_NSWE, TI_WATER_SWE,  TI_WATER_NWE,  TI_WATER_WE,
		TI_WATER_NSE,  TI_WATER_SE,   TI_WATER_NE,   TI_WATER_E,
		TI_WATER_NSW,  TI_WATER_SW,   TI_WATER_NW,   TI_WATER_W,
		TI_WATER_NS,   TI_WATER_S,    TI_WATER_N,    TI_WATER
	},{
		TI_GOO,      TI_GOO_N,    TI_GOO_S,    TI_GOO_NS,
		TI_GOO_W,    TI_GOO_NW,   TI_GOO_SW,   TI_GOO_NSW,
		TI_GOO_E,    TI_GOO_NE,   TI_GOO_SE,   TI_GOO_NSE,
		TI_GOO_WE,   TI_GOO_NWE,  TI_GOO_SWE,  TI_GOO_NSWE
	},{
		TI_THINICE,      TI_THINICE_N,    TI_THINICE_S,    TI_THINICE_NS,
		TI_THINICE_W,    TI_THINICE_NW,   TI_THINICE_SW,   TI_THINICE_NSW,
		TI_THINICE_E,    TI_THINICE_NE,   TI_THINICE_SE,   TI_THINICE_NSE,
		TI_THINICE_WE,   TI_THINICE_NWE,  TI_THINICE_SWE,  TI_THINICE_NSWE
	}};

	ASSERT(wCalcCode < 16);

	return TileImages[wTileType][wCalcCode];
}
#undef IsThinIceTile
