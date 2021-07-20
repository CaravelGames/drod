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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "RoomWidget.h"
#include "DrodBitmapManager.h"
#include "DrodScreen.h"
#include "GameScreen.h"

#include "AumtlichGazeEffect.h"
#include "EvilEyeGazeEffect.h"
#include "GridEffect.h"
#include "ImageOverlayEffect.h"
#include "PendingBuildEffect.h"
#include "RoomEffectList.h"
#include "SnowflakeEffect.h"
#include "RaindropEffect.h"
#include "RoomDrawStatsEffect.h"
#include "SparkEffect.h"
#include "StrikeOrbEffect.h"
#include "TemporalMoveEffect.h"
#include "TileImageCalcs.h"
#include "TileImageConstants.h"
#include "VarMonitorEffect.h"

#include "Light.h"
#include "Rectangle.h"
#include "Sphere.h"

#include <FrontEndLib/Bolt.h>
#include <FrontEndLib/Fade.h>
#include <FrontEndLib/FadeTileEffect.h>
#include <FrontEndLib/FrameRateEffect.h>
#include <FrontEndLib/Pan.h>
#include <FrontEndLib/ShadeEffect.h>
#include <FrontEndLib/FloatEffect.h>
#include <FrontEndLib/TextEffect.h>
#include <FrontEndLib/TransTileEffect.h>

#include "../DRODLib/Aumtlich.h"
#include "../DRODLib/Character.h"
#include "../DRODLib/Citizen.h"
#include "../DRODLib/Clone.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/EvilEye.h"
#include "../DRODLib/FluffBaby.h"
#include "../DRODLib/Gentryii.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/Halph.h"
#include "../DRODLib/PlayerDouble.h"
#include "../DRODLib/Monster.h"
#include "../DRODLib/MonsterPiece.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/Neather.h"
#include "../DRODLib/Serpent.h"
#include "../DRODLib/Slayer.h"
#include "../DRODLib/Spider.h"
#include "../DRODLib/Stalwart.h"
#include "../DRODLib/SettingsKeys.h"
#include "../DRODLib/TemporalClone.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/Db.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

#include <math.h>

#define IS_COLROW_IN_DISP(c,r) \
		(static_cast<UINT>(c) < CDrodBitmapManager::DISPLAY_COLS && \
		static_cast<UINT>(r) < CDrodBitmapManager::DISPLAY_ROWS)

//Change monster frame once every 5 seconds, on average.
const int MONSTER_ANIMATION_DELAY = 5;

#define BOLTS_SURFACE    (0)
#define FOG_SURFACE      (1)
#define CLOUD_SURFACE    (2)
#define SUNSHINE_SURFACE (3)

//Deep/Shallow Water Opacity
#define PIT_DEEP_OPACITY        (50) //Opacity of PIT_MOSAIC drawn in Deep Water
#define PIT_SHALLOW_OPACITY     (25) //Opacity of PIT_MOSAIC drawn in Shallow Water
#define PIT_BOTTOM_OPACITY      (80) //Opacity of FLOOR_DIRT drawn in Shallow Water
#define SKY_DEEP_OPACITY       (128) //Opacity of SkyImage drawn in Deep Water
#define SKY_SHALLOW_OPACITY     (64) //Opacity of SkyImage drawn in Shallow Water
#define SKY_BOTTOM_OPACITY      (80) //Opacity of FLOOR_DIRT drawn in Shallow Water

//Fluff/Thin Ice Opacity
#define FLUFF_OPACITY          (196) //Opacity of Fluff
#define PUFF_OPACITY           (196) //Opacity of Fluff Puffs
#define THIN_ICE_OPACITY       (128) //Opacity of Thin Ice

const Uint8 MAX_FOG_OPACITY = 128; //[0,255]
const Uint8 MIN_FOG_OPACITY =  64; //[0,255]

const SURFACECOLOR SpeakerColor[Speaker_Count] = {
	{255, 255,   0},  //Beethro
	{255, 135,  25},  //Halph
	{255, 192, 255},  //Slayer
	{224, 186, 163},  //Negotiator
	{255, 255, 255},  //(none)
	{255, 255, 255},  //(custom)
	{192, 192, 255},  //Citizen 1
	{255, 128, 255},  //Citizen 2
	{ 64, 255,  64},  //Goblin King
	{ 32, 255,  32},  //Goblin
	{ 64, 255, 255},  //Instructor
	{255, 100,   0},  //Mud Coordinator
	{192, 128,  96},  //Rock golem
	{163, 128, 255},  //Tar Technician
	{255,  96,  96},  //Guard
	{255, 255, 255},  //Evil eye (active)
	{210, 210, 200},  //Citizen
	{255, 255, 128},  //Stalwart
	{160, 160, 160},  //roach
	{192, 192, 192},  //roach queen
	{160, 160, 160},  //roach egg
	{128, 128, 128},  //wwing
	{255, 255, 255},  //evil eye
	{255,  96,  96},  //red serpent
	{ 64,  64, 192},  //tar mother
	{ 64,  64, 192},  //tarbaby
	{255,  32,  32},  //brain
	{ 35, 200, 220},  //mimic
	{ 96, 192, 192},  //spider
	{ 96, 255,  96},  //green serpent
	{128, 128, 255},  //blue serpent
	{255, 128, 128},  //water skipper
	{196, 128, 128},  //water skipper nest
	{220, 220, 220},  //aumtlich
	{255, 255,  28},  //clone
	{160, 160, 160},  //decoy
	{255, 255, 255},  //wubba
	{164, 164, 164},  //seep
	{192, 192,  92},  //fegundo
	{192, 192,  92},  //fegundo ashes
	{192,  64,  64},  //mud mother
	{192,  64,  64},  //mudbaby
	{ 64, 192,  64},  //gel mother
	{ 64, 192,  64},  //gelbaby
	{192, 128,  96},  //rock giant
	{255, 192, 255},  //Citizen 3
	{255, 128, 255},  //Citizen 4
	{255, 255,   0},  //Beethro in disguise
	{255, 192, 255},  //Slayer2
	{255, 135,  25},  //Halph2
	{255, 255, 255},  //(self)
	{255, 200,   0},  //Gunthro
	{255, 255, 255},  //(player)
	{160, 160, 255},  //Soldier
	{220, 220, 210},  //Architect
	{130,  85,  85},  //Construct
	{110, 110, 110},  //Gentryii
	{255, 255,  28},  //temporal clone
	{255, 255, 255}   //Fluff puff
};

//Light resolution.
#define LIGHT_SPT (8)	//light samples per tile in each dimension (NxN)
#define LIGHT_SPT_MINUSONE (LIGHT_SPT-1)	//light cells per tile in each dimension (NxN)
#define LIGHT_BPP (3)   //RGB
const UINT wLightValuesPerTileRow = LIGHT_SPT * LIGHT_BPP;
const UINT wLightValuesPerTile = LIGHT_SPT * wLightValuesPerTileRow;	//NxN RGB
const UINT wLightBytesPerTile = wLightValuesPerTile * sizeof(LIGHTTYPE);
const UINT wLightCellSize[LIGHT_SPT_MINUSONE] = {4,3,3,3,3,3,3};	 //pixels in each sub-tile light cell
const UINT wLightCellPos[LIGHT_SPT_MINUSONE] = {0,4,7,10,13,16,19}; //offset of each sub-tile light cell

//Light palette.
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
const float lightMap[3][NUM_LIGHT_TYPES] = {
	{1.0 ,0.0 ,0.0 ,1.0 ,0.5 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0 ,0.15f,0.65f,0.15f,0.65f,1.0  ,1.0  },
	{1.0 ,0.0 ,1.0 ,0.0 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0 ,0.5 ,0.65f,0.15f,1.0  ,1.0  ,0.15f,0.65f},
	{1.0 ,1.0 ,0.0 ,0.0 ,1.0 ,0.5 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0  ,1.0  ,0.65f,0.15f,0.65f,0.15f}
};
#else
const float lightMap[3][NUM_LIGHT_TYPES] = {
	{1.0 ,1.0 ,0.0 ,0.0 ,1.0 ,0.5 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0  ,1.0  ,0.65f,0.15f,0.65f,0.15f},
	{1.0 ,0.0 ,1.0 ,0.0 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0 ,0.5 ,0.65f,0.15f,1.0  ,1.0  ,0.15f,0.65f},
	{1.0 ,0.0 ,0.0 ,1.0 ,0.5 ,0.5 ,1.0 ,0.5 ,1.0 ,1.0 ,0.15f,0.65f,0.15f,0.65f,1.0  ,1.0  }
};
#endif

//Approximately linearly-scaled darkness.
const float darkMap[NUM_DARK_TYPES] = {
	0.94f, 0.88f, 0.83f, 0.78f, 0.74f, 0.70f, 0.66f, 0.62f,
	0.58f, 0.54f, 0.50f, 0.46f, 0.43f, 0.40f, 0.37f, 0.34f,
	0.31f, 0.29f, 0.27f, 0.25f, 0.23f, 0.21f, 0.19f, 0.17f,
	0.15f, 0.13f, 0.11f, 0.09f, 0.07f, 0.05f, 0.025f, 0.0f
};

//Parameters for ambient light level settings.

//Percent of normal light level to use.
const float fRoomLightLevel[LIGHT_LEVELS] = {1.0f, 0.75f, 0.60f, 0.50f, 0.40f, 0.30f, 0.20f};	   //100% down to 20%
//Maximum light addition.
const float fMaxLightIntensity[LIGHT_LEVELS] = {0.3f, 0.6f, 1.2f, 1.6f, 2.0f, 2.6f, 4.0f};	//30-300% increase
//How many times to emphasize lighting on walls for better look.
const float fWallLightingMultiplier[LIGHT_LEVELS] = {1.5f, 1.75f, 1.9f, 2.0f, 2.1f, 2.3f, 3.0f};
//Since northern walls don't fill a whole tile, this makes light angles look a tiny bit better.
const float Y_LIGHT_OFFSET_KLUDGE = 0.01f;

#define LANTERN_LEVEL (4) //at what darkness level the player uses a lamp

//how much of a tile is lighted by a light source
enum LightedType {L_Dark=0, L_Light=1, L_Partial=2, L_PartialItem=3};
LightedType tileLight[2*MAX_LIGHT_DISTANCE+1][2*MAX_LIGHT_DISTANCE+1];
LightedType subtileLight[LIGHT_SPT][LIGHT_SPT];

//Modelling constants.
const float orbRadius = 0.35f;
const float fNorthWallYCoord = 0.36f;

const SURFACECOLOR BlueGreen = {0, 255, 255};
const SURFACECOLOR Fuschia = {255, 0, 64};
const SURFACECOLOR Orange = {255, 128, 0};
const SURFACECOLOR PaleYellow = {255, 255, 128};
const SURFACECOLOR Red = {255, 0, 0};

//light map index
#define NO_COLOR_INDEX (-1)
#define FROZEN_COLOR_INDEX (1)
#define CLONE_COLOR_INDEX (15)

const UINT CY_RAISED = 4;

const UINT MAX_JITTER = 6; //maximum jitter magnitude (pixels)
const Uint32 JITTER_REDUCTION_INTERVAL = 50; //how often to reduce per-tile jitter magnitude (ms)

//*****************************************************************************
SDL_Rect TileImageBlitParams::display_area = {0,0,0,0};
bool TileImageBlitParams::crop_display = false;

//*****************************************************************************
TileImageBlitParams::TileImageBlitParams(const TileImageBlitParams& rhs)
	: wCol(rhs.wCol), wRow(rhs.wRow)
	, wTileImageNo(rhs.wTileImageNo)
	, wXOffset(rhs.wXOffset), wYOffset(rhs.wYOffset)
	, bDirtyTiles(rhs.bDirtyTiles)
	, bDrawRaised(rhs.bDrawRaised)
	, nOpacity(rhs.nOpacity)
	, bClipped(rhs.bClipped)
	, nAddColor(rhs.nAddColor)
	, bCastShadowsOnTop(rhs.bCastShadowsOnTop)
	, appliedDarkness(rhs.appliedDarkness)
{ }

bool TileImageBlitParams::CropRectToTileDisplayArea(SDL_Rect& BlitRect)
{
	if (!TileImageBlitParams::crop_display)
		return true;
	return CWidget::ClipRectToRect(BlitRect, TileImageBlitParams::display_area);
}

void TileImageBlitParams::setDisplayArea(int x, int y, int w, int h) {
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(w > 0);
	ASSERT(h > 0);

	crop_display = true;
	display_area.x = x;
	display_area.y = y;
	display_area.w = w;
	display_area.h = h;
}

//
//Public methods.
//

//*****************************************************************************
CRoomWidget::CRoomWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH)
	: CWidget(WT_Room, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)

	, pRoomSnapshotSurface(NULL)

	, wDark(0)

	, dwRoomX(0L), dwRoomY(0L)
	, wShowCol(0), wShowRow(0)

	, pCurrentGame(NULL)
	, pRoom(NULL)
	, pTileImages(NULL)
	, bLastVision(false)
	, pActiveLightedTiles(NULL)
	, bRenderRoom(false), bRenderRoomLight(false), bRenderPlayerLight(false)
	, wLastPlayerLightX(UINT(-1)), wLastPlayerLightY(UINT(-1))

	, bShowingPlayer(true)
	, bShowCheckpoints(true)
	, bShowFrameRate(false), bShowMoveCount(false), bShowVarUpdates(false), bShowPuzzleMode(false)
	, bAddNEffect(false)
	, bRequestEvilEyeGaze(false)
	, ghostOpacity(255)
	, temporalCloneEffectHeight(-1)
	, pHighlitMonster(NULL)

	, dwLastDrawSquareInfoUpdateCount(0L)
	, dwLastAnimationFrame(SDL_GetTicks())
	, dwLastMonsterAnimation(0)
	, dwLastBeethroAnimation(0)
	, dwBeethroAnimationFrame(0)

	, dwMovementStepsLeft(0), dwMoveDuration(200), dwCurrentDuration(0)
	, dwLastFrame(0), dwTimeSinceLastFrame(0)
	, bFinishLastMovementNow(false)
	, bAnimateMoves(true), bAnimationInProgress(false)
	, bAllowSleep(false)
	, bNextWispFrame(false)
	, bPlayerSleeping(false)
	, bJitterThisFrame(false), dwLastJitterReduction(0)

	, bOutside(false), bSkyVisible(false)
	, dwSkyX(0)
	, bLightning(false)
	, dwLightning(0)
	, bFog(false)
	, fFogX(0), fFogY(0)
	, fFogOldX(0), fFogOldY(0)
	, fFogVX(0), fFogVY(0)
	, cFogLayer(0)
	, bClouds(false)
	, fCloudX(0), fCloudY(0)
	, fCloudOldX(0), fCloudOldY(0)
	, fCloudAngle(0)
	, bSunlight(false)
	, wSnow(0)
	, rain(0)
	, bSkipLightfade(false)
	, pSkyImage(NULL)

	, bAllDirty(true)
	, bWasPlacingDouble(false)
	, bWasInvisible(false)
	, wLastTurn((UINT)-1)
	, displayFilterOverride(ScriptFlag::D_Nothing)

	, CX_TILE(CBitmapManager::CX_TILE), CY_TILE(CBitmapManager::CY_TILE)

	, fDeathFadeOpacity(0)
	, time_of_last_weather_render(0)
	, redrawingRowForWeather(0)
	, need_to_update_room_weather(false)
	, time_of_last_sky_move(0)
{
	this->imageFilenames.push_back(string("Bolts"));
	this->imageFilenames.push_back(string("Fog1"));
	this->imageFilenames.push_back(string("Clouds1"));
	this->imageFilenames.push_back(string("Sunshine1"));

	this->pLastLayerEffects = new CRoomEffectList(this);
	this->pMLayerEffects = new CRoomEffectList(this);
	this->pOLayerEffects = new CRoomEffectList(this);
	this->pTLayerEffects = new CRoomEffectList(this);

	//To store unchanging room image.
	//It doesn't need to be this large, but it fixes some surface offset issues.
	//Only the area corresponding to the location of this widget will be used.
	ASSERT(!this->pRoomSnapshotSurface);
	this->pRoomSnapshotSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, CScreen::CX_SCREEN, CScreen::CY_SCREEN, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pRoomSnapshotSurface) throw CException();

	RemoveHighlight();
}

//*****************************************************************************
bool CRoomWidget::AddDoorEffect(
//Add an effect to display the affect an orb agent has on a door.
//
//Returns: whether the orb agent is on a door
//
//Params:
	COrbAgentData *pOrbAgent)  //(in) Orb agent to display
{
	const UINT wX = pOrbAgent->wX, wY = pOrbAgent->wY;
	ASSERT(this->pRoom->IsValidColRow(wX, wY));

	//Get door type being affected.
	const UINT wOriginalTileNo = this->pRoom->GetOSquare(wX, wY);

	//Only works for doors and lights.
	const bool bDoor = bIsYellowDoor(wOriginalTileNo);
	const bool bItem = bIsLight(this->pRoom->GetTSquare(wX,wY)) || bIsFiretrap(wOriginalTileNo) ||
		(!bDoor && bIsAnyArrow(this->pRoom->GetFSquare(wX,wY)));
	if (!bDoor && !bItem)
		return false;

	//Contains coords to evaluate.
	CCoordSet drawCoords;
	if (bItem)
		drawCoords.insert(wX,wY);
	else
		this->pRoom->GetAllYellowDoorSquares(wX, wY, drawCoords);

	//Each iteration pops one pair of coordinates for plotting,
	//Exits when there are no more coords in stack to plot.
	UINT wDrawX, wDrawY;
	for (CCoordSet::const_iterator door=drawCoords.begin(); door!=drawCoords.end(); ++door)
	{
		wDrawX = door->wX;
		wDrawY = door->wY;
		ASSERT(this->pRoom->IsValidColRow(wDrawX, wDrawY));
		SetPlot(wDrawX, wDrawY);

		//Plot new tile.
		CCoord coord(wDrawX,wDrawY);
		switch(pOrbAgent->action)
		{
			case OA_NULL:
				//Just highlight the door (mouse is over it).
				AddShadeEffect(wDrawX,wDrawY,PaleYellow);
			break;

			case OA_TOGGLE:
				if (!this->pCurrentGame && !bItem)
					AddLastLayerEffect(new CTransTileEffect(this, coord,
							wOriginalTileNo == T_DOOR_Y ? TI_DOOR_YO : TI_DOOR_Y));
				AddShadeEffect(wDrawX,wDrawY,Orange);
			break;

			case OA_OPEN:
				if (!this->pCurrentGame && !bItem)
					AddLastLayerEffect(new CTransTileEffect(this, coord, TI_DOOR_YO));
				AddShadeEffect(wDrawX,wDrawY,BlueGreen);
			break;

			case OA_CLOSE:
				if (!this->pCurrentGame && !bItem)
					AddLastLayerEffect(new CTransTileEffect(this, coord, TI_DOOR_Y));
				AddShadeEffect(wDrawX,wDrawY,Fuschia);
			break;

			default:
				ASSERT(!"AddDoorEffect: Bad orb agent.");
			break;
		}
	}

	return true;
}

//*****************************************************************************
void CRoomWidget::AddJitter(const CMoveCoord& coord, const float fPercent)
//Adds a specified amount of jitter to the tile.
{
	UINT newJitter = UINT(fPercent * MAX_JITTER);
	newJitter += this->jitterInfo.GetAt(coord.wX, coord.wY);

	if (newJitter < 1) {
		newJitter = 1;
	} else if (newJitter > MAX_JITTER) {
		newJitter = MAX_JITTER;
	}

	this->jitterInfo.Add(coord.wX, coord.wY, newJitter);
}

//*****************************************************************************
void CRoomWidget::AddJitterOffset(
//Apply random jitter, based on the amount specified for this tile.
//
//Params:
	const UINT wX, const UINT wY,
	UINT& wXOffset, UINT& wYOffset) //(in/out)
{
	const UINT jitter = this->jitterInfo.GetAt(wX, wY);
	if (jitter)
	{
		wXOffset += RAND(jitter*2) - jitter;
		wYOffset += RAND(jitter*2) - jitter;
		this->bJitterThisFrame = true;

		JitterBoundsCheck(wX, wY, wXOffset, wYOffset);
	}
}

//*****************************************************************************
void CRoomWidget::JitterBoundsCheck(
//Bounds checking.
//Don't allow jittered sprite to go outside of room area, or light addition
//currently won't work right.
//
//Params:
	const UINT wX, const UINT wY,
	UINT& wXOffset, UINT& wYOffset) //(in/out)
{
	if (wX == 0)
	{
		if ((int)wXOffset < 0)
			wXOffset = 0;
	} else if (wX == this->pRoom->wRoomCols-1) {
		if ((int)wXOffset > 0)
			wXOffset = 0;
	}

	if (wY == 0)
	{
		if ((int)wYOffset < 0)
			wYOffset = 0;
	} else if (wY == this->pRoom->wRoomRows-1) {
		if ((int)wYOffset > 0)
			wYOffset = 0;
	}
}

//*****************************************************************************
void CRoomWidget::AddLayerEffect(CEffect *pEffect, int layer)
{
	switch (layer) {
		case 0:
			AddOLayerEffect(pEffect);
		break;
		case 1:
			AddTLayerEffect(pEffect);
		break;
		case 2:
			AddMLayerEffect(pEffect);
		break;
		case 3:
		default:
			AddLastLayerEffect(pEffect);
		break;
	}
}

//*****************************************************************************
CRoomEffectList* CRoomWidget::GetEffectListForLayer(const int layer) const
{
	switch (layer) {
		case 0: return this->pOLayerEffects;
		case 1: return this->pTLayerEffects;
		case 2: return this->pMLayerEffects;
		case 3: 
		default: return this->pLastLayerEffects;
	}
}

//*****************************************************************************
void CRoomWidget::AddLastLayerEffect(
//Adds an effect to the list of effects drawn after the last layer of the room.
//
//Params:
	CEffect *pEffect) //(in)   Effect to add.  CRoomWidget will take ownership
						//    of the pointer, and caller shouldn't delete.
{
	ASSERT(pEffect);
	this->pLastLayerEffects->AddEffect(pEffect);
}

//*****************************************************************************
void CRoomWidget::AddMLayerEffect(
//Adds an effect to the list of effects drawn after the monster layer of the room.
//
//Params:
	CEffect *pEffect) //(in)   Effect to add.  CRoomWidget will take ownership
						//    of the pointer, and caller shouldn't delete.
{
	ASSERT(pEffect);
	this->pMLayerEffects->AddEffect(pEffect);

	if (getDisplayFilter() != ScriptFlag::D_Normal) {
		//needed for display filter to be applied to these tiles correctly next frame
		this->pMLayerEffects->DirtyTilesForRects(pEffect->dirtyRects);
	}
}

void CRoomWidget::AddOLayerEffect(CEffect *pEffect)
{
	ASSERT(pEffect);
	this->pOLayerEffects->AddEffect(pEffect);

	if (getDisplayFilter() != ScriptFlag::D_Normal) {
		//needed for display filter to be applied to these tiles correctly next frame
		this->pOLayerEffects->DirtyTilesForRects(pEffect->dirtyRects);
	}
}

//*****************************************************************************
void CRoomWidget::AddTLayerEffect(
//Adds an effect to the list of effects drawn after the transparent layer of
//the room.
//
//Params:
	CEffect *pEffect) //(in)   Effect to add.  CRoomWidget will take ownership
						//    of the pointer, and caller shouldn't delete.
{
	ASSERT(pEffect);
	this->pTLayerEffects->AddEffect(pEffect);

	if (getDisplayFilter() != ScriptFlag::D_Normal) {
		//needed for display filter to be applied to these tiles correctly next frame
		this->pTLayerEffects->DirtyTilesForRects(pEffect->dirtyRects);
	}
}

//*****************************************************************************
void CRoomWidget::AddToSubtitles(CSubtitleEffect *pEffect)
//Adds a subtitle effect to the list of subtitles being maintained.
{
	pEffect->AddToSubtitles(this->subtitles);
}

//*****************************************************************************
void CRoomWidget::AddPlayerLight(const bool bAlwaysRefresh) //[default=false]
//Sets a flag placing light on player (if visible), in dark rooms, when room is rendered.
{
	if (!IsPlayerLightRendered())
	{
		//don't need to render any light
		this->wLastPlayerLightX = this->wLastPlayerLightY = UINT(-1);
		return;
	}

	if (bAlwaysRefresh)
	{
		this->bRenderPlayerLight = true;
		return;
	}

	//Use player if in room, otherwise use any NPC Beethro in the room
	ASSERT(this->pCurrentGame);
	CEntity *pPlayer = GetLightholder();

	//Redraw light around player whenever they move in a dark room.
	if (IsPlayerLightShowing() && this->pCurrentGame->wTurnNo != this->wLastTurn)
	{
		if (pPlayer && (pPlayer->wX != this->wLastPlayerLightX || pPlayer->wY != this->wLastPlayerLightY))
			this->bRenderPlayerLight = true;
	}
	//Also set flag to refresh area previously lit by player light when it's no longer rendering.
	if (PlayerLightTurnedOff() && !this->pCurrentGame->swordsman.wPlacingDoubleType) {
		this->bRenderPlayerLight = true;
	}
}

//*****************************************************************************
void CRoomWidget::AddShadeEffect(
//Adds a Shade effect of given color to room tile.
//
//Params:
	const UINT wX, const UINT wY, const SURFACECOLOR &Color) //(in)
{
	if (!this->pRoom->IsValidColRow(wX,wY))
		return;

	CCoord Coord(wX,wY);
	AddLastLayerEffect(new CShadeEffect(this, Coord, Color));
}

//*****************************************************************************
void CRoomWidget::AddStrikeOrbEffect(
//Add a strike orb effect to room.
//
//Params:
	const COrbData &SetOrbData,   //(in) Orb to be struck.
	bool bDrawOrb)
{
	if (bDrawOrb)
		AddTLayerEffect(
			new CFadeTileEffect(this, SetOrbData, TI_ORB_L, 220));
	AddMLayerEffect(
		new CStrikeOrbEffect(this, SetOrbData, this->images[BOLTS_SURFACE], false));
}

//*****************************************************************************
CSubtitleEffect* CRoomWidget::AddSubtitle(
//Adds a line of subtitle text for the given speech command.
//
//Returns: pointer to the subtitle effect this command spawns/adds to
//
//Params:
	CFiredCharacterCommand *pFiredCommand, const Uint32 dwDuration)
{
	CMoveCoord *pCoord = this->pCurrentGame->getSpeakingEntity(pFiredCommand);
	ASSERT(pCoord);

	//Get text.
	ASSERT(pFiredCommand);
	ASSERT(pFiredCommand->pCommand);
	CDbSpeech *pSpeech = pFiredCommand->pCommand->pSpeech;
	ASSERT(pSpeech);
	const WSTRING wStr = pFiredCommand->text;

	//Search for subtitle with this unique pCoord (i.e. object identity, not location).
	SUBTITLES::iterator found = this->subtitles.find(pCoord);
	if (found != this->subtitles.end())
	{
		//Add another line of text to existing subtitle effect.
		CSubtitleEffect *pEffect = found->second;
		pEffect->AddTextLine(wStr.c_str(), dwDuration);
		return pEffect;
	}

	//No subtitle is tied to this object -- instantiate a new subtitle effect.

	//Determine speaker color.
	SPEAKER speaker = SPEAKER(pSpeech->wCharacter);
	switch (speaker)
	{
		case Speaker_Custom:
			 //custom speakers have been resolved in CGameScreen::ProcessSpeechCues
			speaker = Speaker_None;
			break;
		case Speaker_Self:
			 //resolve coloring below
			speaker = Speaker_None;
			break;
		case Speaker_Player:
			//get player's base identity type to determine subtitle color
			speaker = getSpeakerType(MONSTERTYPE(this->pCurrentGame->swordsman.wAppearance));
			break;
		default: break;
	}
	HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacter(speaker);
	SURFACECOLOR color;
	if (speaker != Speaker_None)
	{
		//Use found speaker's color.
		if (pCustomChar) //resolve custom speaker's speaking appearance
			speaker = getSpeakerType(MONSTERTYPE(pCustomChar->wType));
		ASSERT(speaker < Speaker_Count);
		color = SpeakerColor[speaker];
	} else {
		//No speaker is listed:
		//The type of monster speaking determines subtitle background color.
		UINT wIdentity = pFiredCommand->pSpeakingEntity->GetIdentity();
		if (pCustomChar)
			wIdentity = pCustomChar->wType;

		//Speech from no-graphic NPC uses None coloring.
		if (wIdentity >= CUSTOM_CHARACTER_FIRST) //invalid appearance or none
			wIdentity = M_NONE;

		//Speaker appearance must be resolved by now.
		ASSERT(wIdentity < MONSTER_TYPES ||
				(wIdentity >= CHARACTER_FIRST && wIdentity < CHARACTER_TYPES) ||
				wIdentity == M_NONE);

		color = SpeakerColor[getSpeakerType((MONSTERTYPE)wIdentity)];
	}

	//Background color for citizens matches their station-type color.
	CCitizen *pCitizen = dynamic_cast<CCitizen*>(pCoord);
	if (pCitizen)
	{
		const int colorIndex = pCitizen->StationType();
		if (colorIndex >= 0)
		{
			color.byt1 = Uint8((1.0 + lightMap[0][colorIndex]) * 127.5); //half-saturated
			color.byt2 = Uint8((1.0 + lightMap[1][colorIndex]) * 127.5);
			color.byt3 = Uint8((1.0 + lightMap[2][colorIndex]) * 127.5);
		}
	}

	//Speaker text effect.
	CSubtitleEffect *pSubtitle = new CSubtitleEffect(this, pCoord,
			wStr.c_str(), Black, color, dwDuration);
	if (pFiredCommand->bPseudoMonster)
		pSubtitle->FollowCoord(pCoord, true); //delete pseudo monster when subtitle is done
	AddLastLayerEffect(pSubtitle);
	pSubtitle->AddToSubtitles(this->subtitles);
	return pSubtitle;
}

//*****************************************************************************
void CRoomWidget::AddAumtlichGazeEffect(
//Add an aumtlich gaze effect to room.
//
//Params:
      const CMonster *pAumtlich)        //(in) Aumtlich sending out gaze.
{
	CAumtlichGazeEffect *pEffect = new CAumtlichGazeEffect(this, pAumtlich);
	AddMLayerEffect(pEffect);

	//Add sparks where gaze hits.
	if (pEffect->endCoord.wO != NO_ORIENTATION) //if gaze does not exit room
		AddTLayerEffect(new CSparkEffect(this, pEffect->endCoord, 10, true, true));
}

//*****************************************************************************
void CRoomWidget::AllowSleep(const bool bVal)
//Whether player sleep fidget may be shown.
{
	this->bAllowSleep = bVal;
	if (!bVal)
	{
		if (this->bPlayerSleeping)
			StopSleeping();
	}
}

//*****************************************************************************
void CRoomWidget::HandleMouseMotion(
//Handles a mouse motion event.
//
//Params:
	const SDL_MouseMotionEvent &Motion)
{
	if (GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK)
	{
		if (!this->pRoom)
			return;
		if (!GetCurrentGame())
			return;

		PlacePlayerLightAt(Motion.x - this->x, Motion.y - this->y);
	}
}

//*****************************************************************************
void CRoomWidget::PlacePlayerLightAt(int pixel_x, int pixel_y)
{
	ASSERT(this->pRoom);
	ASSERT(GetCurrentGame());

	if (this->pCurrentGame->swordsman.wPlacingDoubleType)
		return; //not supported while placing double

	if (this->wDark >= LANTERN_LEVEL && IsLightingRendered())
	{
		const float fX = pixel_x/float(this->CX_TILE);
		const float fY = pixel_y/float(this->CY_TILE);
		if (this->pRoom->IsValidColRow(UINT(fX),UINT(fY)))
		{
			this->cursorLight.first = fX;
			this->cursorLight.second = fY;
			this->bRenderPlayerLight = true;
			Paint();
		}
	}
}

//*****************************************************************************
void CRoomWidget::HandleMouseUp(const SDL_MouseButtonEvent &Button)
{
	if (!this->pRoom)
		return;
	if (!GetCurrentGame())
		return;

	//Calculate tile clicked on.
	const int pixel_x = Button.x - this->x;
	const int pixel_y = Button.y - this->y;
	const UINT wX = pixel_x / this->CX_TILE;
	const UINT wY = pixel_y / this->CY_TILE;
	if (!this->pRoom->IsValidColRow(wX,wY))
		return;

	if (Button.button == SDL_BUTTON_RIGHT)
	{
		DisplayRoomCoordSubtitle(wX,wY);
		PlacePlayerLightAt(pixel_x, pixel_y);
	}
	else if (Button.button == SDL_BUTTON_LEFT)
	{
		//Interface for mouse-inputted commands.
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
		if (pMonster)
			switch (pMonster->wType)
			{
				case M_CLONE:
				{
					//Switching to a clone.
					CScreen *pScreen = DYN_CAST(CScreen*, CWidget*, this->pParent);
					CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*, pScreen);
					pGameScreen->ProcessCommand(CMD_CLONE, wX, wY);
				}
				return;
				default: break;
			}
		else
		if (this->pCurrentGame->swordsman.wPlacingDoubleType
			&& !this->pCurrentGame->IsPlayerAnsweringQuestions())
		{
			//Placing a double (when there are no unanswered questions taking priority)
			CScreen *pScreen = dynamic_cast<CScreen*>(this->pParent);
			//Allow placing a double with the mouse during an active game session,
			//but not during a demo playback or in a miniroom replay, etc.
			if (pScreen && pScreen->GetScreenType() == SCR_Game)
			{
				CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*, pScreen);
				pGameScreen->ProcessCommand(CMD_DOUBLE, wX, wY);
			}
			return;
		}

		//Highlighting a customized item.
		this->wHighlightX = wX;
		this->wHighlightY = wY;
		this->pHighlitMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wX, wY);
		HighlightSelectedTile();

		//Clicked on player.
		if (this->pCurrentGame->IsPlayerAt(wX,wY) && !this->bPlayerSleeping)
		{
			UINT eSoundID = SEID_NONE;
			switch (this->pCurrentGame->swordsman.wAppearance)
			{
				case M_CLONE: case M_DECOY: case M_MIMIC:
				case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
				case M_TEMPORALCLONE:
					eSoundID = SEID_HI; break;
				case M_GUNTHRO: eSoundID = SEID_GB_HI; break;
				case M_NEATHER: eSoundID = SEID_NLAUGHING; break;
				case M_EYE: case M_EYE_ACTIVE: eSoundID = SEID_EVILEYEWOKE; break;
				case M_GOBLIN:	case M_GOBLINKING: eSoundID = SEID_GOB_HI; break;
				case M_TARBABY: case M_MUDBABY: case M_GELBABY:
				case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
					eSoundID = SEID_SPLAT; break;
				case M_CITIZEN1: case M_CITIZEN2: case M_GUARD:
				case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
				case M_CITIZEN:
					eSoundID = SEID_CIT_HI; break;
				case M_ARCHITECT:
					eSoundID = SEID_ENGINEER_HI; break;
				case M_ROCKGOLEM: case M_ROCKGIANT:
					eSoundID = SEID_ROCK_HI; break;
				case M_CONSTRUCT:
					eSoundID = SEID_CONSTRUCT_HI; break;
				case M_WUBBA: eSoundID = SEID_WUBBA; break;
				case M_HALPH: break; //not supported
				case M_HALPH2: eSoundID = SEID_HALPH2_HI; break;
				case M_SLAYER: break; //not supported
				case M_SLAYER2: eSoundID = SEID_SLAYER_HI; break;
				case M_ROACH: case M_QROACH: case M_WWING: case M_REGG:
				case M_SERPENT: case M_SPIDER: case M_SERPENTG: case M_SERPENTB:
				case M_WATERSKIPPER: case M_AUMTLICH: case M_SEEP: case M_FEGUNDO:
				case M_GENTRYII:
					eSoundID = SEID_MON_CLEAR; break;
				case M_NEGOTIATOR:
					eSoundID = SEID_NEGO_HI; break;
				case M_INSTRUCTOR:
				case M_CITIZEN3: case M_CITIZEN4:
					eSoundID = SEID_WOM_HI; break;
				case M_STALWART:
					eSoundID = SEID_STALWART_HI; break;
				case M_STALWART2:
					eSoundID = SEID_SOLDIER_HI; break;
				default: break;
			}

			if (eSoundID != (UINT)SEID_NONE)
				g_pTheSound->PlaySoundEffect(eSoundID, NULL, NULL, true);
		}
	}
}

//*****************************************************************************
void CRoomWidget::HighlightSelectedTile()
//Context-sensitive highlighting of selected tile (if any).
{
	if (this->pHighlitMonster != NULL)
	{
		this->wHighlightX = this->pHighlitMonster->wX;
		this->wHighlightY = this->pHighlitMonster->wY;
	}
	
	const UINT wX = this->wHighlightX, wY = this->wHighlightY;
	if (!this->pRoom->IsValidColRow(wX,wY))
		return;

	//Reset old display info.
	this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);
	static CMoveCoord briarStats(wX, wY, NO_ORIENTATION);
	SUBTITLES::iterator found = this->subtitles.find(&briarStats);
	if (found != this->subtitles.end())
		found->second->SetToText(NULL, 1);

	ASSERT(this->pCurrentGame);
	const CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
	bool bRemoveHighlightNextTurn = true;
	bool bForceHighlightRemovalNextTurn = false;
	if (pMonster)
	{
		switch (pMonster->wType)
		{
			case M_DECOY:
				DrawInvisibilityRange(wX, wY, NULL);
				bRemoveHighlightNextTurn = false;
			break;

			case M_EYE:
			{
				const CEvilEye *pEye = DYN_CAST(const CEvilEye*, const CMonster*, pMonster);
				if (!pEye->IsAggressive()) {	//show effect if eye is not active
					AddLastLayerEffect(new CEvilEyeGazeEffect(this,wX,wY,pMonster->wO, 2000));
					bForceHighlightRemovalNextTurn = true; //must show only one turn even if coupled with persistent effects
				}
			}
			break;

			case M_GELMOTHER:
			{
				CCoordSet tiles;
				this->pRoom->GetTarConnectedComponent(wX,wY, tiles);
				for (CCoordSet::const_iterator tile = tiles.begin(); tile != tiles.end(); ++tile)
					AddShadeEffect(tile->wX, tile->wY, PaleYellow);
				bRemoveHighlightNextTurn = false;
			}
			break;

			case M_CITIZEN: case M_ARCHITECT:
			{
				//Show citizen's current destination.
				const CCitizen *pCitizen = DYN_CAST(const CCitizen*, const CMonster*, pMonster);
				UINT wDestX, wDestY;
				if (pCitizen->GetGoal(wDestX, wDestY)) {
					AddShadeEffect(wDestX, wDestY, PaleYellow);
					CCoordStack citizenPath = pCitizen->GetStoredPath();
					for (UINT wPathIndex = 0; wPathIndex < citizenPath.GetSize(); ++wPathIndex)
					{
						UINT wX, wY;
						citizenPath.GetAt(wPathIndex, wX, wY);
						ASSERT(this->pRoom->IsValidColRow(wX, wY));
						AddShadeEffect(wX, wY, PaleYellow);
					}
				}
				bRemoveHighlightNextTurn = false;
			}
			break;

			case M_STALWART: case M_STALWART2:
			{
				//Show stalwart's current target.
				CStalwart *pStalwart = DYN_CAST(CStalwart*, CMonster*, const_cast<CMonster*>(pMonster));
				UINT wDestX, wDestY;
				if (!pStalwart->bFrozen && pStalwart->GetGoal(wDestX, wDestY)) {
					CCoordStack stalwartPath = pStalwart->GetStoredPath();
					for (UINT wPathIndex = 0; wPathIndex < stalwartPath.GetSize(); ++wPathIndex)
					{
						UINT wX, wY;
						stalwartPath.GetAt(wPathIndex, wX, wY);
						ASSERT(this->pRoom->IsValidColRow(wX, wY));
						AddShadeEffect(wX, wY, PaleYellow);
					}
				}
				bRemoveHighlightNextTurn = false;
			}
			break;

			case M_SLAYER: case M_SLAYER2:
			{
				static const SURFACECOLOR Purple = {196, 0, 196};
				//Show slayer's wisp.
				const CSlayer *pSlayer = DYN_CAST(const CSlayer*, const CMonster*, pMonster);
				for (const_WISP piece = pSlayer->Pieces.begin(); piece != pSlayer->Pieces.end(); ++piece)
				{
					const CMonsterPiece *pPiece = *piece;
					ASSERT(this->pRoom->IsValidColRow(pPiece->wX, pPiece->wY));
					AddShadeEffect(pPiece->wX, pPiece->wY, Purple);
				}
				bRemoveHighlightNextTurn = false;
			}
			break;

			case M_FLUFFBABY:
			{
				const CFluffBaby *pPuff = DYN_CAST(const CFluffBaby*, const CMonster*, pMonster);
				UINT wDestX, wDestY;
				if (pPuff->GetGoal(wDestX,wDestY))
					AddShadeEffect(wDestX, wDestY, PaleYellow);
				bRemoveHighlightNextTurn = false;
			}
			break;

			case M_HALPH: case M_HALPH2:
			{
				static const SURFACECOLOR Orange = { 255, 165, 0 };
				//Show Halph's stored path
				const CHalph* pHalph = DYN_CAST(const CHalph*, const CMonster*, pMonster);
				CCoordStack halphPath = pHalph->GetStoredPath();
				for (UINT wPathIndex = 0; wPathIndex < halphPath.GetSize(); ++wPathIndex)
				{
					UINT wX, wY;
					halphPath.GetAt(wPathIndex, wX, wY);
					ASSERT(this->pRoom->IsValidColRow(wX, wY));
					AddShadeEffect(wX, wY, Orange);
				}
				bRemoveHighlightNextTurn = false;
			}
			break;

			default: break;
		}
	}

	switch (this->pRoom->GetTSquare(wX,wY))
	{
		case T_ORB:
		{
			const COrbData *pOrb = this->pRoom->GetOrbAtCoords(wX,wY);
			if (pOrb)
				for (UINT wIndex=pOrb->agents.size(); wIndex--; )
					AddDoorEffect(pOrb->agents[wIndex]);
			bRemoveHighlightNextTurn = false;
		}
		break;
		case T_BOMB:
			HighlightBombExplosion(wX, wY, T_BOMB);
			bRemoveHighlightNextTurn = false;
		break;
		case T_POWDER_KEG:
			HighlightBombExplosion(wX, wY, T_POWDER_KEG);
			bRemoveHighlightNextTurn = false;
		break;
		case T_BRIAR_SOURCE:
		{
			//How many turns until this root grows: edge pieces / roots.
			const UINT index = this->pRoom->briars.getIndexAt(wX,wY);
			ASSERT(index); //every root should have an index
			const UINT numRoots = this->pRoom->briars.getNumSourcesWithIndex(index);
			const CCoordSet& tiles = this->pRoom->briars.getEdgeTilesFor(wX,wY);
			const UINT numEdgeTiles = tiles.size();

			WSTRING wstr;
			WCHAR temp[16];
			if (numRoots > 1)
			{
				wstr += _itow(numEdgeTiles, temp, 10);
				wstr += wszForwardSlash;
				wstr += _itow(numRoots, temp, 10);
				wstr += wszEqual;
			}
			wstr += _itow(numEdgeTiles/numRoots, temp, 10);

			briarStats.wX = wX;
			briarStats.wY = wY;
			AddInfoSubtitle(&briarStats, wstr, 0); //indefinite
			bRemoveHighlightNextTurn = false;
		}
		break;
		case T_BRIAR_LIVE: case T_BRIAR_DEAD:
		{
			//Highlight this briar's connected component.
			const CCoordSet& tiles = this->pRoom->briars.getEdgeTilesFor(wX,wY);
			for (CCoordSet::const_iterator tile = tiles.begin(); tile != tiles.end(); ++tile)
				AddShadeEffect(tile->wX, tile->wY, PaleYellow);
			bRemoveHighlightNextTurn = false;
		}
		break;
		case T_HORN_SQUAD:
		{
			UINT goalX, goalY;
			if (this->pCurrentGame->GetNearestEntranceForHorn(wX, wY, M_CLONE, goalX, goalY)) {
				AddShadeEffect(goalX, goalY, PaleYellow);
				bRemoveHighlightNextTurn = false;
			}
		}
		break;
		case T_HORN_SOLDIER:
		{
			UINT goalX, goalY;
			if (this->pCurrentGame->GetNearestEntranceForHorn(wX, wY, M_STALWART2, goalX, goalY)) {
				SURFACECOLOR Blue = { 120, 120, 255 };
				AddShadeEffect(goalX, goalY, Blue);
				bRemoveHighlightNextTurn = false;
			}
		}
		break;
		default: break;
	}

	const UINT wOSquare = this->pRoom->GetOSquare(wX,wY);
	switch (wOSquare)
	{
		//Highlight tunnel exit.
		case T_TUNNEL_E: case T_TUNNEL_W: case T_TUNNEL_N: case T_TUNNEL_S:
		{
			int dx=0, dy=0;
			switch (wOSquare)
			{
				case T_TUNNEL_E: dx = 1; dy = 0; break;
				case T_TUNNEL_W: dx = -1; dy = 0; break;
				case T_TUNNEL_N: dx = 0; dy = -1; break;
				case T_TUNNEL_S: dx = 0; dy = 1; break;
				default: ASSERT(!"Invalid tunnel type"); break;
			}
			UINT wExitX, wExitY;
			if (this->pCurrentGame->TunnelGetExit(wX, wY, dx, dy, wExitX, wExitY))
				AddShadeEffect(wExitX, wExitY, PaleYellow);
			bRemoveHighlightNextTurn = false;
		}
		break;
		case T_PRESSPLATE:
		{
			COrbData *pPlate = this->pRoom->GetPressurePlateAtCoords(wX,wY);
			if (pPlate)
				for (UINT wIndex=pPlate->agents.size(); wIndex--; )
					AddDoorEffect(pPlate->agents[wIndex]);
			bRemoveHighlightNextTurn = false;
		}
		break;
		case T_DOOR_Y: case T_DOOR_YO:
			DisplayDoorAgents(wX, wY);
			bRemoveHighlightNextTurn = false;
		break;
		case T_FIRETRAP: case T_FIRETRAP_ON:
			DisplayAgentsAffectingTiles(CCoordSet(wX, wY));
			bRemoveHighlightNextTurn = false;
		break;
		case T_FLUFFVENT:
			if (this->pRoom->GetTSquare(wX,wY) == T_FLUFF)
			{
				CCoordSet tiles;
				this->pRoom->GetTarConnectedComponent(wX,wY, tiles);
				for (CCoordSet::const_iterator tile = tiles.begin(); tile != tiles.end(); ++tile)
					AddShadeEffect(tile->wX, tile->wY, PaleYellow);
				bRemoveHighlightNextTurn = false;
			}
		break;
		default: break;
	}

	const UINT wFTile = this->pRoom->GetFSquare(wX,wY);
	if (bIsAnyArrow(wFTile)){
		DisplayAgentsAffectingTiles(CCoordSet(wX, wY));
		bRemoveHighlightNextTurn = false;
	}

	if (bRemoveHighlightNextTurn) {
		RemoveHighlight();
	} else if (bForceHighlightRemovalNextTurn) {
		this->pHighlitMonster = NULL;
		this->wHighlightX = this->wHighlightY = UINT(-1);
	}
}

//*****************************************************************************
void CRoomWidget::DisplayAgentsAffectingTiles(const CCoordSet& doorCoords)
{
	for (UINT orb=0; orb < this->pRoom->orbs.size(); ++orb)
	{
		COrbData* pData = this->pRoom->orbs[orb];
		for (UINT agent=0; agent < pData->agents.size(); ++agent)
		{
			COrbAgentData* pAgentData = pData->agents[agent];
			if (doorCoords.has(pAgentData->wX, pAgentData->wY))
			{
				//Show effect on triggering agent itself.
				COrbAgentData sourceAgent(pData->wX, pData->wY, pAgentData->action);
				AddOrbEffect(&sourceAgent);
			}
		}
	}
}

//*****************************************************************************
void CRoomWidget::DisplayDoorAgents(
//Finds all orbs acting on this door and
//displays visual representation in the room widget.
//
//Params:
	const UINT wX, const UINT wY) //(in) square door is on
{
	CCoordSet doorCoords;
	this->pRoom->GetAllYellowDoorSquares(wX, wY, doorCoords);

	DisplayAgentsAffectingTiles(doorCoords);
}

//*****************************************************************************
void CRoomWidget::AddInfoSubtitle(
	CMoveCoord* pCoord, const WSTRING& wstr, const Uint32 dwDuration,
	const UINT displayLines, const SDL_Color& color, const UINT fadeDuration) //default=[1,Black,500]
{
	ASSERT(pCoord);

	SUBTITLES::iterator found = this->subtitles.find(pCoord);
	if (found != this->subtitles.end())
	{
		//Add text to existing subtitle effect.
		found->second->AddTextLine(wstr.c_str(), dwDuration, color);
	} else {
		CSubtitleEffect *pEffect = new CSubtitleEffect(this, pCoord, wstr.c_str(),
				color, SpeakerColor[Speaker_None], dwDuration, displayLines);
		pEffect->SetFadeDuration(fadeDuration);
		AddLastLayerEffect(pEffect);
		pEffect->AddToSubtitles(this->subtitles);
	}
}

//*****************************************************************************
bool CRoomWidget::AddOrbEffect(
//Add an effect to display the affect an orb agent has on a door.
//
//Returns: whether the orb agent is on a door
//
//Params:
	COrbAgentData *pOrbAgent)  //(in) Orb agent to display
{
	const UINT wX = pOrbAgent->wX, wY = pOrbAgent->wY;
	ASSERT(this->pRoom->IsValidColRow(wX, wY));

	//Only for orbs and pressure plates.
	const bool bPressPlate = this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE;
	if (this->pRoom->GetTSquare(wX, wY) != T_ORB && !bPressPlate)
		return false;

	//Set highlight color.
	SURFACECOLOR color;
	switch (pOrbAgent->action)
	{
		case OA_NULL:
			//Just highlight the door (mouse is over it).
			color = PaleYellow;
		break;
		case OA_TOGGLE: color = Orange; break;
		case OA_OPEN: color = BlueGreen; break;
		case OA_CLOSE: color = Red; break;
		default:
			ASSERT(!"AddOrbEffect: Bad orb agent.");
		break;
	}

	//Get set of all tiles to highlight.
	CCoordSet tiles(wX,wY);
	if (bPressPlate)
	{
		const CTileMask plateMask(T_PRESSPLATE);
		this->pRoom->GetConnected4NeighborTiles(wX, wY, plateMask, tiles);
	}

	for (CCoordSet::const_iterator tile=tiles.begin(); tile!=tiles.end(); ++tile)
		AddShadeEffect(tile->wX, tile->wY, color);

	return true;
}

//*****************************************************************************
void CRoomWidget::ClearEffects(
//Clears all effects in the room.
//
//Params:
	const bool bKeepInfoTexts) //(in) If true (default), info text effects will persist
	                           //     and old effects will be erased from room
{
	SynchRoomToCurrentGame();

	//If bKeepInfoTexts is true, then erase (i.e. redraw) effect areas also
	//because the caller is going to keep showing the room.
	const bool bRepaint = bKeepInfoTexts;

	//Clear layer lists except for persistent display effects.
	this->pOLayerEffects->Clear(bRepaint, !bKeepInfoTexts);
	this->pTLayerEffects->Clear(bRepaint, !bKeepInfoTexts);
	this->pMLayerEffects->Clear(bRepaint, !bKeepInfoTexts);
	this->pLastLayerEffects->Clear(bRepaint, !bKeepInfoTexts);

	RemoveHighlight();

	ResetJitter();

	//If these effects were removed, then reset their display flags.
	if (!bKeepInfoTexts)
		this->puzzleModeOptions.bIsEnabled = this->bShowFrameRate = this->bShowMoveCount = this->bShowVarUpdates = this->bShowPuzzleMode = false;
}

//*****************************************************************************
void CRoomWidget::ClearLights()
//Zeroes all light arrays.
{
	ASSERT(this->pRoom);
	this->lightMaps.reset();
	this->lightedPlayerTiles.clear();
	this->lightedRoomTiles.clear();
	this->partialLightedTiles.clear();

	this->tileLightInfo.Init(this->pRoom->wRoomCols, this->pRoom->wRoomRows);

	this->jitterInfo.Init(this->pRoom->wRoomCols, this->pRoom->wRoomRows);
}

//*****************************************************************************
void CRoomWidget::DisplayChatText(const WSTRING& text, const SDL_Color& color)
//Display chat text as a subtitle in the corner of the room.
{
	static CMoveCoord coord(0, 0, NO_ORIENTATION);
	if (this->pCurrentGame) {
		coord.wX = this->pCurrentGame->swordsman.wX >= this->pRoom->wRoomCols/2 ?
				0 : this->pRoom->wRoomCols-1;
		coord.wY = this->pCurrentGame->swordsman.wY >= this->pRoom->wRoomRows/2 ?
				0 : this->pRoom->wRoomRows-1;
	} else {
		coord.wX = coord.wY = 0;
	}

	//When displaying many new chat texts at the same time,
	//extend the display duration up to 30s.
	static const UINT MAX_DISPLAY_DURATION = 30000; //ms
	static const UINT CHAT_FADE_DURATION = 5000;
	static const UINT CHAT_LINES = 7;
	UINT displayDuration = 15000;
	SUBTITLES::iterator found = this->subtitles.find(&coord);
	if (found != this->subtitles.end())
	{
		//Add text to existing subtitle effect.
		displayDuration += found->second->GetDisplayTimeRemaining();
	}
	if (displayDuration > MAX_DISPLAY_DURATION)
		displayDuration = MAX_DISPLAY_DURATION;

	AddInfoSubtitle(&coord, text, displayDuration, CHAT_LINES, color, CHAT_FADE_DURATION);

	//Don't offset subtitle effect by any tiles to appear more compact.
	found = this->subtitles.find(&coord);
	if (found != this->subtitles.end())
	{
		CSubtitleEffect& effect = *(found->second);
		effect.SetEffectType(ECHATTEXT);
		effect.SetOffset(0, 0);
		effect.RequestRetainOnClear();
	}
}

//*****************************************************************************
void CRoomWidget::DisplayRoomCoordSubtitle(const int nMouseX, const int nMouseY)
//Display room coords as a subtitle, based on mouse's screen position.
{
	const UINT wX = (nMouseX - this->x) / this->CX_TILE;
	const UINT wY = (nMouseY - this->y) / this->CY_TILE;
	DisplayRoomCoordSubtitle(wX,wY);
}

//*****************************************************************************
void CRoomWidget::DisplayRoomCoordSubtitle(const UINT wX, const UINT wY)
//Display room coords at (x,y), along with what item is there, as a subtitle.
{
	if (!this->pRoom->IsValidColRow(wX,wY)) return;

	static CMoveCoord coord(wX, wY, NO_ORIENTATION);
	coord.wX = wX;
	coord.wY = wY;

	WSTRING wstr = wszLeftParen;
	WCHAR temp[12];
	wstr += _itoW(wX, temp, 10);
	wstr += wszComma;
	wstr += _itoW(wY, temp, 10);
	wstr += wszRightParen;

	//Identify items on tile.
#define AppendLine(mid) if (mid) {wstr += wszCRLF; wstr += g_pTheDB->GetMessageText(mid);}
	UINT mid = 0;

	//Player.
	if (this->pCurrentGame && this->pCurrentGame->IsPlayerAt(wX,wY))
	{
		AppendLine(MID_Player);
	}

	//Monster.
	const CMonster* pMonster = this->pRoom->pFirstMonster;
	int index = 1;
	while (pMonster)
	{
		const CCharacter *pCharacter = dynamic_cast<const CCharacter*>(pMonster);
		bool bCharacterName = false;
		bool bShowMoveOrder = pMonster->IsVisible();
		if (pCharacter) {
			if (!pCharacter->IsVisible() && !pCharacter->IsInvisibleInspectable()) {
				goto SkipDescribingMonster;
			}

			bShowMoveOrder = bShowMoveOrder || pCharacter->IsInvisibleCountMoveOrder();

			if (pMonster->wX != wX || pMonster->wY != wY)
				goto SkipDescribingMonster;

			if (pCharacter->GetCustomName() != DefaultCustomCharacterName) {
				wstr += wszCRLF;
				wstr += pCharacter->GetCustomName();
				bCharacterName = true;

			} else if (pCharacter->wLogicalIdentity >= CUSTOM_CHARACTER_FIRST) {
				//Show custom character name.
				ASSERT(this->pRoom);
				if (this->pCurrentGame) {
					ASSERT(this->pCurrentGame->pHold);
					const HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacter(pCharacter->wLogicalIdentity);
					if (pCustomChar) {
						bCharacterName = true;
						wstr += wszCRLF;
						wstr += pCustomChar->charNameText;
					}
				}
			} else {
				bCharacterName = true;
				AppendLine(TILE_MID[T_CHARACTER]);
				mid = getMIDForMonster(pCharacter->wIdentity);
				if (mid) {
					wstr += wszSpace;
					wstr += g_pTheDB->GetMessageText(mid);
				}
			}
		}

		if (pMonster->wX != wX || pMonster->wY != wY)
			goto SkipDescribingMonster;

		if (!bCharacterName) {
			mid = getMIDForMonster(pMonster->wType);
			AppendLine(mid);
		}

		//Indicate citizen color.
		if (pMonster->wType == M_CITIZEN)
		{
			const CCitizen *pCitizen = DYN_CAST(const CCitizen*, const CMonster*, pMonster);
			const int stationType = pCitizen->StationType();
			if (stationType >= 0)
			{
				wstr += wszSpace;
				wstr += wszPoundSign;
				wstr += _itoW(stationType, temp, 10);
			}
		}

		//Indicate serpent length.
		if (bIsSerpentOrGentryii(pMonster->wType))
		{
			int snakeLength;
			if (pMonster->wType == M_GENTRYII)
			{
				snakeLength = 1 + pMonster->Pieces.size();
			} else {
				const CSerpent *pSerpent = DYN_CAST(const CSerpent*, const CMonster*, pMonster);
				snakeLength = pSerpent->GetLength();
			}
			wstr += wszSpace;
			wstr += wszLeftBracket;
			wstr += _itoW(snakeLength, temp, 10);
			wstr += g_pTheDB->GetMessageText(MID_SnakeLengthAffix);
			wstr += wszRightBracket;
		}

		//Indicate monster's position in movement order sequence.
		if (bShowMoveOrder) {
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += wszPoundSign;
			wstr += _itoW(index, temp, 10);
			wstr += wszRightParen;
		}

SkipDescribingMonster:
		//Only count monsters that are in the room
		if (bShowMoveOrder)
			++index;

		pMonster = pMonster->pNext;
	}

	//Items.
	const UINT tTile = this->pRoom->GetTSquare(wX, wY);
	if (tTile != T_EMPTY)
	{
		switch (tTile)
		{
			case T_TOKEN:
				mid = GetTokenMID(this->pRoom->GetTParam(wX, wY)); break;
			case T_ORB:
			{
				const COrbData *pOrb = this->pRoom->GetOrbAtCoords(wX, wY);
				mid = GetOrbMID(pOrb ? pOrb->eType : OT_NORMAL);
				break;
			}
			default: mid = TILE_MID[tTile]; break;
		}
		AppendLine(mid);

		//Indicate station color.
		if (tTile == T_STATION)
		{
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += wszPoundSign;
			wstr += _itoW(this->pRoom->GetTParam(wX, wY), temp, 10);
			wstr += wszRightParen;
		}
	}

	// T-Covered tiles.
	const UINT tCoveredTile = this->pRoom->GetCoveredTSquare(wX, wY);
	if (tCoveredTile != T_EMPTY)
	{
		switch (tCoveredTile)
		{
			case T_TOKEN:
				mid = GetTokenMID(this->pRoom->GetTParam(wX, wY)); break;
			default: mid = TILE_MID[tCoveredTile]; break;
		}
		AppendLine(mid);
	}

	//Flat layer.
	const UINT fTile = this->pRoom->GetFSquare(wX, wY);
	if (fTile != 0)
	{
		mid = TILE_MID[fTile];
		AppendLine(mid);
		if (bIsArrow(fTile) || bIsDisabledArrow(fTile))
		{
			wstr += wszSpace;
			wstr += wszLeftParen;

			UINT arrowmid = 0;
			switch (fTile)
			{
				case T_ARROW_N:  case T_ARROW_OFF_N:  arrowmid = MID_North; break;
				case T_ARROW_NE: case T_ARROW_OFF_NE: arrowmid = MID_NorthEast; break;
				case T_ARROW_E:  case T_ARROW_OFF_E:  arrowmid = MID_East; break;
				case T_ARROW_SE: case T_ARROW_OFF_SE: arrowmid = MID_SouthEast; break;
				case T_ARROW_S:  case T_ARROW_OFF_S:  arrowmid = MID_South; break;
				case T_ARROW_SW: case T_ARROW_OFF_SW: arrowmid = MID_SouthWest; break;
				case T_ARROW_W:  case T_ARROW_OFF_W:  arrowmid = MID_West; break;
				case T_ARROW_NW: case T_ARROW_OFF_NW: arrowmid = MID_NorthWest; break;
			}
			ASSERT(arrowmid);
			wstr += g_pTheDB->GetMessageText(arrowmid);
			wstr += wszRightParen;
		}
	}
	if (this->pRoom->checkpoints.has(wX, wY))
	{
		mid = TILE_MID[T_CHECKPOINT];
		AppendLine(mid);
	}

	//Always describe o-layer.
	const UINT oTile = this->pRoom->GetOSquare(wX, wY);
	switch (oTile)
	{
		case T_PRESSPLATE:
		{
			COrbData *pPlate = this->pRoom->GetPressurePlateAtCoords(wX,wY);
			mid = GetPressurePlateMID(pPlate ? pPlate->eType : OT_NORMAL);
		}
		break;
		default: mid = TILE_MID[oTile]; break;
	}

	AppendLine(mid);

	//Build marker.
	if (this->pRoom->building.get(wX,wY))
	{
		UINT wBuildTile = this->pRoom->building.get(wX, wY) - 1;

		if (bIsFakeTokenType(wBuildTile)) {
			mid = GetTokenMID(ConvertFakeTokenType(wBuildTile));
		}	else {
			mid = getBuildMarkerTileMID(wBuildTile);
		}

		if (mid)
		{
			wstr += wszCRLF;
			wstr += g_pTheDB->GetMessageText(MID_BuildMarker);
			wstr += wszColon;
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(mid);
		}
	}
#undef AppendLine

	AddInfoSubtitle(&coord, wstr, 2000);
}

//*****************************************************************************
void CRoomWidget::DisplaySubtitle(
//Displays a notification at (x,y) that room exiting is locked.
//
//
	const WCHAR *pText, const UINT wX, const UINT wY, const bool bReplace)
{
	if (!this->pRoom->IsValidColRow(wX,wY)) return;
	if (!pText) return;

	static CMoveCoord coord(wX, wY, NO_ORIENTATION);
	CMoveCoord *pCoord = &coord;
	coord.wX = wX;
	coord.wY = wY;

	SUBTITLES::iterator found = this->subtitles.find(pCoord);
	static const int dwDuration = 2000;
	if (found != this->subtitles.end())
	{
		//Replace text or add another line of text to existing subtitle effect.
		if (bReplace)
			found->second->SetToText(pText, dwDuration);
		else
			found->second->AddTextLine(pText, dwDuration);
	} else {
		CSubtitleEffect *pEffect = new CSubtitleEffect(this, pCoord, pText,
				Black, SpeakerColor[Speaker_None], dwDuration, 1);
		AddLastLayerEffect(pEffect);
		pEffect->AddToSubtitles(this->subtitles);
	}
}

//*****************************************************************************
void CRoomWidget::DontAnimateMove()
//Call if the game move just played should not be animated.
{
	if (this->pCurrentGame)
	{
		this->wLastTurn = this->pCurrentGame->wTurnNo;
		SetMoveCountText();
	}
	BetterVisionQuery();
	FinishMoveAnimation();
}

//*****************************************************************************
UINT CRoomWidget::GetCustomEntityTile(
//Determines what sprite tile to show for this custom character role.
//
//Returns: index of custom sprite to show, or TI_UNSPECIFIED if not defined
//
//Params:
	const UINT wLogicalIdentity,      //role's logical ID
	const UINT wO, const UINT wFrame) //orientation and frame number
const
{
	//Check for a custom character tileset.
	if (this->pCurrentGame)
	{
		ASSERT(this->pCurrentGame->pHold);
		const HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacterConst(wLogicalIdentity);
		return GetCustomEntityTileFor(pCustomChar, wO, wFrame);
	}

	return TI_UNSPECIFIED; //no custom role tiles provided
}

//*****************************************************************************
UINT CRoomWidget::GetCustomEntityTileFor(
	const HoldCharacter *pCustomChar,
	const UINT wO, const UINT wFrame) //orientation and frame number
const
{
	UINT wCustomTileImageNo = TI_UNSPECIFIED; //no custom role tiles provided by default
	if (pCustomChar)
	{
		const UINT wIndex = GetCustomTileIndex(wO);
		if (pCustomChar->animationSpeed)
		{
			//This indicates to display the tiles in the corresponding custom tileset in sequence,
			//ignoring the frame# passed in above.
			const UINT animationFrame = SDL_GetTicks() / pCustomChar->animationSpeed;
			wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, wIndex, animationFrame, true);
			if (wCustomTileImageNo == TI_UNSPECIFIED && wIndex > 0) //get first orientation frame if this one is unavailable
				wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, 0, animationFrame, true);
		} else {
			wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, wIndex, wFrame);
			//Robust frame fetching if not all frames are available in this custom tileset.
			if (wCustomTileImageNo == TI_UNSPECIFIED && wFrame > 0) //allow having only one animation frame
				wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, wIndex, 0);
			if (wCustomTileImageNo == TI_UNSPECIFIED && wIndex > 0) //get first orientation frame
				wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, 0, wFrame);
			if (wCustomTileImageNo == TI_UNSPECIFIED && wFrame > 0 && wIndex > 0) //get only frame available
				wCustomTileImageNo = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, 0, 0);
		}
	}

	return wCustomTileImageNo;
}

//*****************************************************************************
UINT CRoomWidget::GetCustomTileIndex(const UINT wO)
//Returns: custom image tile index for a given orientation
{
	switch (wO)
	{
		case N: return 0;
		case NE: return 1;
		case E: return 2;
		case SE: return 3;
		case S: return 4;
		case SW: return 5;
		case W: return 6;
		case NW: return 7;
		case NO_ORIENTATION: return 8;
		default: ASSERT(!"Bad orientation"); return 0;
	}
}

//*****************************************************************************
UINT CRoomWidget::GetEntityTile(
//Determines what sprite tile to show for this character role.
//
//Params:
	const UINT wApparentIdentity,     //what stock entity the role looks like
	const UINT wLogicalIdentity,      //role's logical ID
	const UINT wO, const UINT wFrame) //orientation and frame number
const
{
	const UINT tile = GetCustomEntityTile(wLogicalIdentity, wO, wFrame);
	if (tile != TI_UNSPECIFIED)
		return tile;

	return GetStockEntityTile(wApparentIdentity, wO, wFrame);
}

//*****************************************************************************
UINT CRoomWidget::GetStockEntityTile(
	const UINT wIdentity, const UINT wO, const UINT wFrame)
const
{
	switch (wIdentity) {
		case M_BEETHRO:
		case M_TEMPORALCLONE:
		case M_CLONE:
			return GetTileImageForBeethro(wO, wFrame, this->dwBeethroAnimationFrame);
		default:
			return GetTileImageForEntity(wIdentity == M_NONE ?
				static_cast<UINT>(CHARACTER_FIRST) : wIdentity, wO, wFrame);
	}
}

//*****************************************************************************
void CRoomWidget::GetWeather()
//Get environmental conditions from room data.
{
	if (this->pCurrentGame)
		this->pRoom = this->pCurrentGame->pRoom; //ensure rooms are synched
	ASSERT(this->pRoom);

	if (IsLightingRendered())
	{
		this->wDark = this->pRoom->weather.wLight;
		ASSERT(this->wDark < LIGHT_LEVELS);
		g_pTheDBM->fLightLevel = fRoomLightLevel[this->wDark];
	} else {
		//Full lighting
		this->wDark = 0;
		g_pTheDBM->fLightLevel = 1.0;
	}

	while (!this->playThunder.empty())
		this->playThunder.pop();
	if (!IsWeatherRendered())
	{
		//No weather effects on lower graphics modes.
		this->bOutside = this->bSkyVisible = this->bLightning = this->bClouds =
				this->bSunlight = this->bSkipLightfade = this->bFog = false;
		this->cFogLayer = this->wSnow = this->rain = 0;
		return;
	}

	this->bAllDirty = true;

	this->bOutside = this->pRoom->weather.bOutside;
	this->bSkyVisible = this->bOutside && SkyWillShow();

	this->bLightning = this->pRoom->weather.bLightning;

	this->bClouds = this->pRoom->weather.bClouds;
	ASSERT(this->images[CLOUD_SURFACE]);

	this->bSunlight = this->pRoom->weather.bSunshine;
	ASSERT(this->images[SUNSHINE_SURFACE]);

	this->bSkipLightfade = this->pRoom->weather.bSkipLightfade;

	this->cFogLayer = (BYTE)this->pRoom->weather.wFog;
	ASSERT(this->cFogLayer < FOG_INCREMENTS);
	this->bFog = this->cFogLayer > 0;
	if (this->bFog)
	{
		ASSERT(this->images[FOG_SURFACE]);
		EnableSurfaceBlending(this->images[FOG_SURFACE], 128);
	}

	this->wSnow = this->pRoom->weather.wSnow;
	ASSERT(this->wSnow < SNOW_INCREMENTS);

	this->sky = this->pRoom->weather.sky;

	this->rain = this->pRoom->weather.rain;
	ASSERT(this->rain < RAIN_INCREMENTS);
}

//*****************************************************************************
CEntity* CRoomWidget::GetLightholder() const
//Returns: pointer to entity holding a light (lamp) in the room.
//This is the player by default.  However, if the player is not in the room,
//then any NPC Beethro/Gunthro has a light.
{
	if (this->pCurrentGame->swordsman.IsInRoom())
		return &this->pCurrentGame->swordsman;
	return this->pCurrentGame->pRoom->GetNPCBeethro();
}

//*****************************************************************************
bool CRoomWidget::IsLightingRendered() const
//Returns: whether light level effects should be rendered
{
	return g_pTheBM->bAlpha && !this->puzzleModeOptions.GetHideLighting();
}

//*****************************************************************************
bool CRoomWidget::IsPlayerLightShowing() const
//Returns: true if there is a player/NPC Beethro light to show in the room
{
	ASSERT(this->pCurrentGame);
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom()) //if player is in room, light shows when visible
		return player.IsVisible();
	//When player is not in room, any visible NPC Beethro shows light
	if (this->pCurrentGame->pRoom->GetNPCBeethro() != NULL)
		return true;
	return false;
}

//*****************************************************************************
bool CRoomWidget::IsPlayerLightRendered() const
//Returns: true if light is shown around player in the front end.
//Light will be showing in dark rooms when light graphics preferences are enabled.
{
	if (!this->bShowingPlayer)
		return false; //don't show light if player is explicitly hidden

	return this->wDark >= LANTERN_LEVEL && IsLightingRendered() && IsWeatherRendered();
}

//*****************************************************************************
bool CRoomWidget::IsWeatherRendered() const
//Returns: whether weather effects should be rendered
{
	return g_pTheBM->eyeCandy > 0;
}

//*****************************************************************************
bool CRoomWidget::ShowShadowCastingAnimation() const
{
	if (g_pTheBM->eyeCandy >= 8)
		return true;

	return this->pCurrentGame->playerLightType > 0;
}

//*****************************************************************************
bool CRoomWidget::IsTemporalCloneLightRendered() const
{
	return g_pTheBM->eyeCandy >= 12;
}

//*****************************************************************************
bool CRoomWidget::LoadFromCurrentGame(
//Loads widget from current game.
//
//Params:
	CCurrentGame *pSetCurrentGame, const bool bLoad) //[default=true]
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pSetCurrentGame);
	this->pCurrentGame = pSetCurrentGame;
	return LoadFromRoom(pSetCurrentGame->pRoom, bLoad);
}

//*****************************************************************************
bool CRoomWidget::LoadFromRoom(
//Loads widget from current game.
//
//Params:
	CDbRoom *pRoom, //room to display
	const bool bLoad) //[default=true]
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pRoom);
	this->pRoom = pRoom;  //quick access

	//Update vars used for comparison of widget to current game.
	this->style = this->pRoom->style;
	this->dwRoomX = this->pRoom->dwRoomX;
	this->dwRoomY = this->pRoom->dwRoomY;

	//Reset animations.
	this->dwCurrentDuration = this->dwLastFrame = this->dwTimeSinceLastFrame = 0;
	this->wLastTurn = UINT(-1);

	ClearEffects();

	//Load tile images.
	bool bSuccess = true;
	if (bLoad)
	{
		bSuccess = g_pTheDBM->LoadTileImagesForStyle(this->style);
		LoadRoomImages();	//must do before ResetForPaint
	}

	//Set tile image arrays to new current room.  Render room.
	ResetForPaint();

	return bSuccess;
}

//*****************************************************************************
void CRoomWidget::LoadRoomImages()
//Load images specific to this room.
{
	//Query environmental conditions.
	GetWeather();
	if (!this->rain) //raindrop effects should not persist into a non-rainy room
		this->pMLayerEffects->RemoveEffectsOfType(ERAINDROP);

	ClearLights();

	if (g_pTheDBM->pTextures[FLOOR_IMAGE])
		SDL_FreeSurface(g_pTheDBM->pTextures[FLOOR_IMAGE]);
	if (g_pTheDBM->pTextures[OVERHEAD_IMAGE])
		SDL_FreeSurface(g_pTheDBM->pTextures[OVERHEAD_IMAGE]);

	//Load new floor/overhead image textures, if any.
	CDbRoom *pThisRoom = this->pCurrentGame ? this->pCurrentGame->pRoom : this->pRoom;
	g_pTheDBM->pTextures[FLOOR_IMAGE] = g_pTheDBM->LoadImageSurface(pThisRoom->dwDataID);
	g_pTheDBM->pTextures[OVERHEAD_IMAGE] = g_pTheDBM->LoadImageSurface(pThisRoom->dwOverheadDataID);

	//Generate room model for lighting.
	this->bRenderRoomLight = true;
	this->bCeilingLightsRendered = false;

	//Load sky image, if applicable.
	if (this->bSkyVisible)
	{
		//Get name of sky image.  Look up default name for this style+light level if none specified.
		WSTRING wstrSkyImageName = this->sky;
		if (wstrSkyImageName.empty())
		{
			WSTRING style = pThisRoom->style;
			g_pTheDBM->ConvertStyle(style);
			style += wszSpace;
			style += wszSKIES;
			list<WSTRING> skies;
			if (CFiles::GetGameProfileString(INISection::Graphics, style.c_str(), skies) && !skies.empty())
			{
				//Sky images specified.  Choose the one closest to specified light level.
				UINT wSkyIndex = skies.size() > this->wDark ? this->wDark : skies.size()-1;
				list<WSTRING>::const_iterator sky = skies.begin();
				while (wSkyIndex--)
					++sky;
				wstrSkyImageName = *sky;
			}
		}

		if (wstrSkyImageName.empty())
		{
			//No sky image listed.
			if (this->pSkyImage)
			{
				SDL_FreeSurface(this->pSkyImage);
				this->pSkyImage = NULL;
			}
			this->wstrSkyImage.resize(0);
		} else {
			//If image name is different than the one already loaded, load new sky.
			if (wstrSkyImageName.compare(this->wstrSkyImage))
			{
				if (this->pSkyImage)
					SDL_FreeSurface(this->pSkyImage);
				this->pSkyImage = g_pTheDBM->LoadImageSurface(wstrSkyImageName.c_str(), 0);
				if (!this->pSkyImage)
					this->wstrSkyImage = wszEmpty;
				else
				{
					ASSERT((UINT)this->pSkyImage->w == this->w);
					ASSERT((UINT)this->pSkyImage->h == this->h);
					this->wstrSkyImage = wstrSkyImageName;
				}
			}
		}
	}
}

//*****************************************************************************
bool CRoomWidget::PlayerLightTurnedOff() const
//Returns: true if player was showing a light the last time rendered, but not this turn
{
	return !this->lightedPlayerTiles.empty() &&
		(!IsPlayerLightShowing() || this->pCurrentGame->playerLightType < 0);
}

//*****************************************************************************
void CRoomWidget::ProcessCueEventsBeforeRoomDraw(const CCueEvents &CueEvents)
//Processes Cue Events
{
	if (this->pHighlitMonster != NULL)
	{
		if (CueEvents.HasOccurredWith(CID_MonsterExistenceCeased, this->pHighlitMonster))
		{
			this->RemoveHighlight();
		}
	}
}

//*****************************************************************************
void CRoomWidget::SetMoveCountText()
{
	if (!this->bShowMoveCount || !this->pCurrentGame)
		return;
	CEffect *pEffect = this->pLastLayerEffects->GetEffectOfType(ETEXT);
	if (!pEffect)
		return;

	//Don't set the text if there is a different text effect onscreen now.
	CTextEffect *pMoveCount = DYN_CAST(CTextEffect*, CEffect*, pEffect);
	if (pMoveCount->X() == 0 && pMoveCount->Y() == 0)
	{
		WCHAR moveNum[10];
		_itoW(this->pCurrentGame->wPlayerTurn, moveNum, 10);
		pMoveCount->SetText(moveNum, FONTLIB::F_FrameRate);
	}
}

//*****************************************************************************
void CRoomWidget::ShowFrameRate(const bool bVal)
//Shows/hides frame rate as a widget drawn on top everything.
{
	ASSERT(this->bShowFrameRate != bVal);
	this->bShowFrameRate = bVal;
	if (bVal)
	{
		AddLastLayerEffect(new CFrameRateEffect(this));
		AddLastLayerEffect(new CRoomDrawStatsEffect(this));

		//Don't have overlapping display info.
		if (this->bShowMoveCount)
			ShowMoveCount(false);
	} else {
		this->pLastLayerEffects->RemoveEffectsOfType(EFRAMERATE);
		this->pLastLayerEffects->RemoveEffectsOfType(EROOMDRAWSTATS);
	}
}

//*****************************************************************************
void CRoomWidget::ShowMoveCount(const bool bVal)
//Shows/hides move count as a widget drawn on top everything.
{
	ASSERT(this->bShowMoveCount != bVal);
	this->bShowMoveCount = bVal;
	if (bVal)
	{
		CTextEffect *pMoveCount = new CTextEffect(this, wszEmpty, FONTLIB::F_FrameRate,
				0, (UINT)-1, false, true);
		pMoveCount->RequestRetainOnClear(true);
		pMoveCount->Move(0,0);
		AddLastLayerEffect(pMoveCount);
		SetMoveCountText();

		//Don't have overlapping display info.
		if (this->bShowFrameRate)
			ShowFrameRate(false);
	} else {
		this->pLastLayerEffects->RemoveEffectsOfType(ETEXT);
	}
}


//*****************************************************************************
void CRoomWidget::SetPuzzleModeOptions(const PuzzleModeOptions &puzzleModeOptions)
//Enables puzzle mode (display for easier puzzle solving).
{
	this->puzzleModeOptions = puzzleModeOptions;
	this->puzzleModeOptions.bIsEnabled = this->bShowPuzzleMode;

	if (this->bShowPuzzleMode) {
		// If puzzle mode is currently active we need to turn it off and on again to refresh it fully
		TogglePuzzleMode();
		TogglePuzzleMode();
	}
}
//*****************************************************************************
void CRoomWidget::ShowPuzzleMode(const bool bVal)
//Enables puzzle mode (display for easier puzzle solving).
{
	if (this->bShowPuzzleMode == bVal)
		return;
	
	this->RerenderRoom();
	this->bShowPuzzleMode = bVal;
	this->bRenderPlayerLight = true;
	this->bRenderRoomLight = true;
	this->puzzleModeOptions.bIsEnabled = this->bShowPuzzleMode;

	if (bVal)
	{
		if (this->puzzleModeOptions.GetShowGrid()) {
			CGridEffect* pEffect = new CGridEffect(this, this->puzzleModeOptions.wGridStyle, this->puzzleModeOptions.uGridOpacity);
			pEffect->RequestRetainOnClear(true);
			AddLastLayerEffect(pEffect);
		}

		if (this->puzzleModeOptions.GetHideLighting())
			this->wDark = 0;

		if (this->puzzleModeOptions.GetHideWeather()) {
			RemoveMLayerEffectsOfType(ESNOWFLAKE);
			RemoveMLayerEffectsOfType(ERAINDROP);
		}
	} else {
		this->pLastLayerEffects->RemoveEffectsOfType(EGRID);

		this->wDark = this->pRoom->weather.wLight;
	}

	g_pTheDBM->fLightLevel = fRoomLightLevel[this->wDark];

	BetterVisionQuery();
	UpdateDrawSquareInfo();
}

//*****************************************************************************
void CRoomWidget::ShowVarUpdates(const bool bVal)
//Shows when hold vars change state.
{
	if (this->bShowVarUpdates == bVal)
		return;
	this->bShowVarUpdates = bVal;

	if (bVal)
	{
		CVarMonitorEffect *pEffect = new CVarMonitorEffect(this);
		pEffect->SetText(wszAsterisk);
		AddLastLayerEffect(pEffect);
	} else {
		this->pLastLayerEffects->RemoveEffectsOfType(EVARMONITOR);
	}
}

//*****************************************************************************
void CRoomWidget::ToggleFrameRate()
//Shows/hides frame rate.
{
	ShowFrameRate(!this->bShowFrameRate);
}

//*****************************************************************************
void CRoomWidget::ToggleMoveCount()
//Shows/hides current move count.
{
	ShowMoveCount(!this->bShowMoveCount);
}

//*****************************************************************************
void CRoomWidget::TogglePuzzleMode()
//Toggles puzzle display mode.
{
	ShowPuzzleMode(!this->bShowPuzzleMode);
}

//*****************************************************************************
void CRoomWidget::ToggleVarDisplay()
//Shows/hides frame rate.
{
	ShowVarUpdates(!this->bShowVarUpdates);
}

//*****************************************************************************
void CRoomWidget::DisplayPersistingImageOverlays(CCueEvents& CueEvents)
{
	if (!this->pCurrentGame)
		return;

	if (this->pCurrentGame->persistingImageOverlays.empty())
		return;

	//Don't duplicate the persistent effects drawn above when we get to ProcessImageEvents.
	static const CUEEVENT_ID cid = CID_ImageOverlay;
	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(cid);
	while (pObj)
	{
		const CImageOverlay *pImageOverlay = DYN_CAST(const CImageOverlay*, const CAttachableObject*, pObj);

		if (pImageOverlay->loopsForever()) {
			// These effects were already added to `persistingImageOverlays`
			CueEvents.Remove(cid, pObj);
			pObj = CueEvents.GetFirstPrivateData(cid);
			continue;
		}

		//Don't wait for additional images to be added to the room widget before clearing effects.
		bool bClearedEffect = false;
		const int clearsLayer = pImageOverlay->clearsImageOverlays();
		if (clearsLayer != ImageOverlayCommand::NO_LAYERS) {
			RemoveLayerEffects(EIMAGEOVERLAY, clearsLayer);
			CueEvents.Remove(cid, pObj);
			pObj = CueEvents.GetFirstPrivateData(cid);
			bClearedEffect = true;
		}

		const int clearsGroup = pImageOverlay->clearsImageOverlayGroup();
		if (clearsGroup != ImageOverlayCommand::NO_GROUP) {
			RemoveGroupEffects(clearsGroup);
			bClearedEffect = true;
		}

		if (bClearedEffect)
			continue;

		pObj = CueEvents.GetNextPrivateData();
	}

	const UINT currentTurn = this->pCurrentGame->wTurnNo;
	const Uint32 dwNow = CScreen::dwCurrentTicks;

	for (vector<CImageOverlay>::const_iterator it=this->pCurrentGame->persistingImageOverlays.begin();
			it!=this->pCurrentGame->persistingImageOverlays.end(); ++it)
	{
		const CImageOverlay& overlay = *it;
		const int layer = overlay.getLayer();
		CEffectList *pEffectList = GetEffectListForLayer(layer);
		if (!IsPersistentEffectPlaying(pEffectList, overlay.instanceID)) {
			CImageOverlayEffect *pEffect = new CImageOverlayEffect(this, &overlay, currentTurn, dwNow);
			AddLayerEffect(pEffect, layer);
		}
	}
}

//*****************************************************************************
bool CRoomWidget::IsPersistentEffectPlaying(CEffectList* pEffectList, const UINT instanceID) const
{
	ASSERT(pEffectList);
	std::list<CEffect*> pEffects = pEffectList->Effects;
	for (std::list<CEffect*>::const_iterator it=pEffects.begin(); it!=pEffects.end(); ++it)
	{
		const CEffect *pEffect = *it;
		if (pEffect->GetEffectType() == EIMAGEOVERLAY) {
			const CImageOverlayEffect *pIOE = DYN_CAST(const CImageOverlayEffect*, const CEffect*, pEffect);
			if (pIOE->getInstanceID() == instanceID)
				return true;
		}
	}
	return false;
}

//*****************************************************************************
void CRoomWidget::FadeToLightLevel(
//Crossfades room image from old to new light level.
//
//Params:
	const UINT wNewLight,
	CCueEvents& CueEvents)
{
	StopSleeping();

	if (wNewLight == this->wDark)
		return; //no fade needed

	const UINT wLightDelta = abs(int(this->wDark) - int(wNewLight));
	this->wDark = wNewLight;
	g_pTheDBM->fLightLevel = fRoomLightLevel[this->wDark];

	SDL_Surface *pOldRoomSurface = NULL, *pNewRoomSurface = NULL;

	//Query fade duration.
	string str;
	Uint32 dwFadeDuration = 500/2; //default
	if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::RoomTransitionSpeed, str))
		dwFadeDuration = atoi(str.c_str()) / 2;
	dwFadeDuration *= wLightDelta;

	if (!IsLightingRendered())
		dwFadeDuration = 0; //don't fade at all if light level is always full

	//Prepare and perform crossfade if duration is non-zero.
	if (dwFadeDuration)
	{
		//Image of room at previous light level.
		pOldRoomSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		//Image of room at new light level.
		pNewRoomSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	}

	//If for some reason a surface couldn't be allocated,
	//the new room will be shown without any transition effect.
	if (!pOldRoomSurface || !pNewRoomSurface)
	{
		if (pOldRoomSurface)
			SDL_FreeSurface(pOldRoomSurface);
		if (pNewRoomSurface)
			SDL_FreeSurface(pNewRoomSurface);

		//Just show at new light level.
		ClearEffects();
		UpdateFromCurrentGame();
		Paint();
	} else {
		//Take snapshot of room.
		SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_Rect tempRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
		SDL_BlitSurface(GetDestSurface(), &rect, pOldRoomSurface, &tempRect);

		//Prepare and render new image of room (update this->pRoomSnapshotSurface).
		LoadRoomImages();
		AddPlayerLight(IsPlayerLightRendered());
		DirtyRoom();
		UpdateDrawSquareInfo();

		SetEffectsFrozen(true);
		RenderRoomInPlay(this->wShowCol, this->wShowRow);
		SDL_BlitSurface(this->pRoomSnapshotSurface, &rect, pNewRoomSurface, &tempRect);

		//Render room objects.
		this->x = this->y = 0;
		DisplayPersistingImageOverlays(CueEvents);
		RenderRoomLayers(pNewRoomSurface);
		this->x = rect.x;
		this->y = rect.y;

		//Crossfade.
		{
			CFade fade(pOldRoomSurface, pNewRoomSurface);
			SDL_Surface *pSrcSurface = fade.GetDestSurface();
			SDL_Surface *pDestSurface = GetDestSurface();
			Uint32 dwNow = SDL_GetTicks();
			const UINT dwStart = dwNow;
			const Uint32 dwEnd = dwStart + dwFadeDuration;
			while (dwNow < dwEnd)
			{
				dwNow = SDL_GetTicks();
				fade.IncrementFade((dwNow - dwStart) / (float)dwFadeDuration);
				SDL_BlitSurface(pSrcSurface, &tempRect, pDestSurface, &rect);
				UpdateRect();
				g_pTheBM->UpdateRects(pDestSurface);
			}
		}

		//Done.
		SDL_FreeSurface(pOldRoomSurface);
		SDL_FreeSurface(pNewRoomSurface);
		SetEffectsFrozen(false);
	}
}

//*****************************************************************************
void CRoomWidget::RenderRoomLayers(SDL_Surface* pSurface, const bool bDrawPlayer)
{
	ASSERT(this->pRoom);

	RenderFogInPit(pSurface);
	DrawPlatformsAndTLayer(pSurface);
	this->pTLayerEffects->DrawEffects(pSurface, EIMAGEOVERLAY);

	if (bDrawPlayer && this->pCurrentGame)
		DrawPlayer(this->pCurrentGame->swordsman, pSurface);
	DrawMonsters(this->pRoom->pFirstMonster, pSurface, false);

	this->pMLayerEffects->DrawEffects(pSurface, EIMAGEOVERLAY);

	DrawOverheadLayer(pSurface);
	DrawGhostOverheadCharacters(pSurface, false);
	
	this->pLastLayerEffects->DrawEffects(pSurface, EIMAGEOVERLAY);

	RenderEnvironment(pSurface);

	ApplyDisplayFilterToRoom(getDisplayFilter(), pSurface);
}

//*****************************************************************************
void CRoomWidget::ShowRoomTransition(
//Changes view from old room to current one.
//
//Params:
	const UINT wExitOrientation,  //(in) direction of exit
	CCueEvents& CueEvents)
{
	StopSleeping();

	//Load new floor image mosaic, if any.
	LoadRoomImages();

	string str;
	Uint32 dwPanDuration = 500; //default
	if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::RoomTransitionSpeed, str))
		dwPanDuration = atoi(str.c_str());

	//Make panning speed consistent along either axis.
	if (wExitOrientation == W || wExitOrientation == E)
		dwPanDuration = static_cast<UINT>(dwPanDuration * (this->pRoom->wRoomCols / float(this->pRoom->wRoomRows)));

	if (!IsValidOrientation(wExitOrientation) || wExitOrientation == NO_ORIENTATION ||
			!dwPanDuration) //skip effect if value is zero
	{
		//Just show new room.
		ClearEffects();
		UpdateFromCurrentGame();
		Paint();
		return;
	}

	// We don't want effects to lose "animation" time while the transition happens
	SetEffectsFrozen(true);

	//Show a smooth transition between rooms.
	PanDirection panDirection = PanNorth;
	switch (wExitOrientation)
	{
		case N: panDirection = PanNorth; break;
		case S: panDirection = PanSouth; break;
		case E: panDirection = PanEast;  break;
		case W: panDirection = PanWest;  break;
		default: ASSERT(!"Bad pan direction.");  break;
	}

	//Image of room being left.
	SDL_Surface *pOldRoomSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	//Image of room being entered.
	SDL_Surface *pNewRoomSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	SDL_Rect tempRect = MAKE_SDL_RECT(0, 0, this->w, this->h);

	//If for some reason a surface couldn't be allocated,
	//the new room will be shown without any transition effect.
	if (!pOldRoomSurface || !pNewRoomSurface)
	{
		if (pOldRoomSurface)
			SDL_FreeSurface(pOldRoomSurface);
		if (pNewRoomSurface)
			SDL_FreeSurface(pNewRoomSurface);
	} else {
		//Take snapshot of room being left.
		SDL_BlitSurface(GetDestSurface(), &rect, pOldRoomSurface, &tempRect);

		//Render image of new room (update this->pRoomSnapshotSurface).
		ClearEffects();
		UpdateFromCurrentGame();
		ASSERT(this->pRoomSnapshotSurface);
		SDL_BlitSurface(this->pRoomSnapshotSurface, &rect,
					pNewRoomSurface, &tempRect);

		this->x = this->y = 0;
		DisplayPersistingImageOverlays(CueEvents);
		RenderRoomLayers(pNewRoomSurface, false);

		this->x = rect.x;
		this->y = rect.y;

		//Slide from the old to new room.
		CPan pan(pOldRoomSurface, pNewRoomSurface, panDirection);
		const UINT dwStart = SDL_GetTicks();
		SDL_Surface *pDestSurface = GetDestSurface();
		static const Uint32 FRAMES_PER_SECOND = 60;
		static const Uint32 frame_duration = 1000 / FRAMES_PER_SECOND;
		Uint32 last_update = 0;
		for (;;)
		{
			Uint32 dwNow = SDL_GetTicks();

			//Cap transition framerate.
			while (dwNow < last_update + frame_duration) {
				SDL_Delay(1);
				dwNow = SDL_GetTicks();
			}
			last_update = dwNow;

			const Uint32 totalMSPassed = dwNow - dwStart;
			pan.IncrementPan(totalMSPassed / (float)dwPanDuration);
			SDL_BlitSurface(pOldRoomSurface, &tempRect,
					pDestSurface, &rect);
			UpdateRect();
			g_pTheBM->UpdateRects(pDestSurface);
			if (totalMSPassed >= dwPanDuration)
			{
				//Done panning.
				SDL_FreeSurface(pOldRoomSurface);
				SDL_FreeSurface(pNewRoomSurface);
				break;
			}
		}
	}

	// And now the effects can continue
	SetEffectsFrozen(false);
}

//*****************************************************************************
void CRoomWidget::StopSleeping()
//Stops all effects related to the player sleeping during idle time.
{
	this->bPlayerSleeping = false;
	RemoveMLayerEffectsOfType(EFLOAT);
	g_pTheSound->StopSoundEffect(SEID_SNORING);
}

//*****************************************************************************
bool CRoomWidget::SubtitlesHas(CSubtitleEffect *pEffect) const
//Returns: whether the specified effect is in the set of active subtitles
{
	for (SUBTITLES::const_iterator iter = this->subtitles.begin();
			iter != this->subtitles.end(); ++iter)
	{
		if (pEffect == iter->second)
			return true;
	}
	return false;
}

//*****************************************************************************
void CRoomWidget::UpdateFromCurrentGame(
//Update the room widget so that it is ready to display the room from
//the current game.
	const bool bForceReload) //whether to force reloading the graphics style
{
	//If the room changed, then get the new one.
	ASSERT(this->pCurrentGame);
	SynchRoomToCurrentGame();

	//Room widget can only display standard-sized rooms now.
	ASSERT(this->pRoom->wRoomCols == CDrodBitmapManager::DISPLAY_COLS);
	ASSERT(this->pRoom->wRoomRows == CDrodBitmapManager::DISPLAY_ROWS);

	this->style = this->pRoom->style;
	VERIFY(g_pTheDBM->LoadTileImagesForStyle(this->style, bForceReload));
	if (bForceReload)
		LoadRoomImages(); //ensure these are current with reloaded style

	//Prepare new room.
	AddPlayerLight();
	RenderRoomLighting();
	DirtyRoom();
	if (this->dwRoomX != this->pRoom->dwRoomX ||
		this->dwRoomY != this->pRoom->dwRoomY)
	{
		this->dwRoomX = this->pRoom->dwRoomX;
		this->dwRoomY = this->pRoom->dwRoomY;
	}
	VERIFY(UpdateDrawSquareInfo());

	//Redraw the room.
	RenderRoomInPlay(this->wShowCol, this->wShowRow);
}

//*****************************************************************************
void CRoomWidget::UpdateFromPlots(const CCoordSet *pSet, const CCoordSet *pGeometryChanges)
//Refresh the tile image arrays after plots have been made.
{
	//Next call will recalc the indicated tile images and their neighbors.
	UpdateDrawSquareInfo(pSet, pGeometryChanges);
}

//*****************************************************************************
void CRoomWidget::ResetForPaint()
//Reset the object so that the next paint will draw everything fresh with no
//assumptions about what is already drawn in the widget area.
{
	this->bAllDirty = true;
	this->wLastTurn = UINT(-1);
	this->dwTimeSinceLastFrame = this->dwMoveDuration;
	this->monsterAnimations.clear();

	SynchRoomToCurrentGame();

	UpdateDrawSquareInfo();
}

//*****************************************************************************
void CRoomWidget::BlitDirtyRoomTiles(const bool bMoveMade)
//Redraw all tiles in room that need refreshing.
{
	const UINT wStartPos = this->wShowRow * this->pRoom->wRoomCols + this->wShowCol;
	const UINT wRowOffset = this->pRoom->wRoomCols - CDrodBitmapManager::DISPLAY_COLS;
	const UINT wXEnd = this->wShowCol + CDrodBitmapManager::DISPLAY_COLS;
	const UINT wYEnd = this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS;

	SDL_Surface *pDestSurface = GetDestSurface();
	SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	SDL_Rect dest = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	TileImages *pbMI = this->pTileImages + wStartPos, *pbRMI;
	UINT wX, wY;
	for (wY = this->wShowRow; wY < wYEnd; ++wY)
	{
		for (wX = this->wShowCol; wX < wXEnd; ++wX)
		{
			if (pbMI->dirty || (bMoveMade && pbMI->monster))
			{
				src.x = dest.x = this->x + (wX - this->wShowCol) * CX_TILE;
				src.y = dest.y = this->y + (wY - this->wShowRow) * CY_TILE;
				SDL_BlitSurface(this->pRoomSnapshotSurface, &src, pDestSurface, &dest);
				pbMI->damaged = 1;   //tile must be updated on screen
				if (bMoveMade && pbMI->monster && wY > this->wShowRow)
				{
					//Monster was possibly raised.  Redraw the tile above it also.
					src.y -= CY_TILE;
					dest.y -= CY_TILE;
					SDL_BlitSurface(this->pRoomSnapshotSurface, &src, pDestSurface, &dest);
					pbRMI = pbMI - this->pRoom->wRoomCols;
					pbRMI->damaged = 1;   //tile must be updated on screen
					pbRMI->dirty = pbRMI->monster = 0;     //tile is no longer dirty
				}
				pbMI->dirty = pbMI->monster = 0;     //tile is no longer dirty
			}
			++pbMI;
		}
		pbMI += wRowOffset;
	}
}

//*****************************************************************************
void CRoomWidget::CountDirtyTiles(UINT& damaged, UINT& dirty, UINT& monster) const
{
	damaged = dirty = monster = 0;

	const UINT wStartPos = this->wShowRow * this->pRoom->wRoomCols + this->wShowCol;
	const UINT wXEnd = this->wShowCol + CDrodBitmapManager::DISPLAY_COLS;
	const UINT wYEnd = this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS;

	TileImages *pbMI = this->pTileImages + wStartPos;
	UINT wX, wY;
	for (wY = this->wShowRow; wY < wYEnd; ++wY)
	{
		for (wX = this->wShowCol; wX < wXEnd; ++wX)
		{
			if (pbMI->damaged)
				++damaged;
			if (pbMI->dirty)
				++dirty;
			if (pbMI->monster)
				++monster;
			++pbMI;
		}
	}
}

//*****************************************************************************
void CRoomWidget::DrawTileEdges(
//Draws black edges around a tile if needed.
//
//Params:
	const UINT wX, const UINT wY, //(in) Tile coords.
	const TileImages* pTI,        //(in) Edge data for tile.
	SDL_Surface *pDestSurface)    //(in) Where to draw edges.
{
	static const SURFACECOLOR EdgeColor[EDGE_COUNT] = {
		{(Uint8)-1, (Uint8)-1, (Uint8)-1}, {96, 96, 96}, {0, 0, 0}};
	static const UINT CX_TILE = CBitmapManager::CX_TILE;
	static const UINT CY_TILE = CBitmapManager::CY_TILE;
	static const UINT DISPLAY_COLS = CDrodBitmapManager::DISPLAY_COLS;
	static const UINT DISPLAY_ROWS = CDrodBitmapManager::DISPLAY_ROWS;

	const int nX = this->x + wX * CX_TILE, nY = this->y + wY * CY_TILE;

	if (pTI->edges.north)
		DrawRow(nX, nY, CX_TILE, EdgeColor[pTI->edges.north], pDestSurface);
	//join some cases at the corner so it looks better
	else if (wY)
	{
		if (wX)
		{
			if ((pTI - DISPLAY_COLS)->edges.west && (pTI - 1)->edges.north &&
					!(pTI - DISPLAY_COLS)->edges.south && !(pTI - 1)->edges.east) //don't overlap two corners
				DrawPixel(nX, nY, EdgeColor[(pTI-1)->edges.north], pDestSurface);
		}
		if (wX < DISPLAY_COLS-1)
		{
			if ((pTI - DISPLAY_COLS)->edges.east && (pTI + 1)->edges.north &&
					!(pTI - DISPLAY_COLS)->edges.south && !(pTI + 1)->edges.west)
				DrawPixel(nX + CX_TILE-1, nY, EdgeColor[(pTI+1)->edges.north], pDestSurface);
		}
	}

	if (pTI->edges.south)
		DrawRow(nX, nY + CY_TILE-1, CX_TILE, EdgeColor[pTI->edges.south], pDestSurface);
	//join some cases at the corner so it looks better
	else if (wY < DISPLAY_ROWS-1)
	{
		if (wX)
		{
			if ((pTI + DISPLAY_COLS)->edges.west && (pTI - 1)->edges.south &&
					!(pTI + DISPLAY_COLS)->edges.north && !(pTI - 1)->edges.east)
				DrawPixel(nX, nY + CY_TILE-1, EdgeColor[(pTI-1)->edges.south], pDestSurface);
		}
		if (wX < DISPLAY_COLS-1)
		{
			if ((pTI + DISPLAY_COLS)->edges.east && (pTI + 1)->edges.south &&
					!(pTI + DISPLAY_COLS)->edges.north && !(pTI + 1)->edges.west)
				DrawPixel(nX + CX_TILE-1, nY + CY_TILE-1,
						EdgeColor[(pTI+1)->edges.south], pDestSurface);
		}
	}

	if (pTI->edges.west)
		DrawCol(nX, nY, CY_TILE, EdgeColor[pTI->edges.west], pDestSurface);

	if (pTI->edges.east)
		DrawCol(nX + CX_TILE-1, nY, CY_TILE, EdgeColor[pTI->edges.east], pDestSurface);

	//Special edging around the inner wall texture.
	static const int halfWallY = CY_TILE/2;
	if (pTI->edges.bHalfWall)
	{
		//South-facing
		DrawRow(nX, nY + halfWallY, CX_TILE, EdgeColor[2], pDestSurface);
	} else {
		//East- or west-facing
		const bool bNotInnerWall = GetWallTypeAtSquare(this->pRoom, wX, wY) != WALL_INNER;
		if (wX > 0 && (pTI-1)->edges.bHalfWall && bNotInnerWall)
			DrawCol(nX, nY + halfWallY, CY_TILE - halfWallY, EdgeColor[2], pDestSurface);
		if (wX+1 < this->pRoom->wRoomCols && (pTI+1)->edges.bHalfWall && bNotInnerWall)
			DrawCol(nX + CX_TILE-1, nY + halfWallY, CY_TILE - halfWallY, EdgeColor[2], pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::RenderRoomInPlay(
//Repaints the "physical room" (the o- and t-layers) within the given rectangle.
//Uses "smart" room pre-rendering, only painting tiles that have changed since
//last call.
//
//Params:
	const int wCol, const int wRow,     //(in) top-left tile coords
	const int wWidth, const int wHeight)
{
	ASSERT(this->pCurrentGame);
	const CSwordsman& player = this->pCurrentGame->swordsman;

	//Check for state changes that will affect entire room.
	const bool bIsPlacingDouble = player.wPlacingDoubleType != 0;
	const bool bIsInvisible = !player.IsVisible();
	const bool bPlayerIsDying = this->pCurrentGame->IsPlayerDying();
	const bool bStateChanged = (this->bWasPlacingDouble != bIsPlacingDouble) ||
		(this->bWasInvisible && !bIsInvisible) || bPlayerIsDying;
	if (bStateChanged)
		this->bAllDirty = true;

	//1. Apply current light calculations.
	if (this->bRenderPlayerLight) //render player light when flagged
	{
		//Copy room lighting to display buffer.
		this->lightMaps.copy(this->lightMaps.psDisplayedLight, this->lightMaps.psRoomLight);

		RenderPlayerLight();

		//Done rendering player lighting.
		this->bRenderPlayerLight = false;
	}

	//2. Draw o- and t-layers.
	RenderRoom(wCol, wRow, wWidth, wHeight, false);

	this->bWasPlacingDouble = bIsPlacingDouble;
	this->bWasInvisible = bIsInvisible;
}

//*****************************************************************************
//Calculate lighting for a source originating from the player's position.
//This is done by caching lighting information in a buffer,
//then adding this buffer's info to the overall lighting to be displayed in the room.
void CRoomWidget::RenderPlayerLight()
{
	//Reset entity tile lighting from last render.
	ResetPlayerLightMap();

	//Render light map for this frame.
	const bool bLightIsOff = PlayerLightTurnedOff();
	if (bLightIsOff)
	{
		this->wLastPlayerLightX = this->wLastPlayerLightY = UINT(-1);
	} else {
		PropagatePlayerLight();

		//Could propagate light from other entities at this point if desired,
		//adding their light to this buffer.

		//Propagate light from user cursor to see room better.
		if (this->pRoom->IsValidColRow(UINT(this->cursorLight.first),UINT(this->cursorLight.second)))
		{
			static const UINT wCursorLightRadius = 5;
			static const UINT wLightParam = (wCursorLightRadius-1)*NUM_LIGHT_TYPES+0; //white light
			const float fZ = getTileElev(this->pRoom->GetOSquareWithGuessing(
				int(this->cursorLight.first + 0.5f), int(this->cursorLight.second + 0.5f)));
			PropagateLight(this->cursorLight.first, this->cursorLight.second, fZ, wLightParam, false);
		}
	}

	//Add the entity light buffer to the display buffer.
	const UINT wLastCol = this->pRoom->wRoomCols-1;
	const UINT wLastRow = this->pRoom->wRoomRows-1;
	for (CCoordSet::const_iterator tile = this->lightedPlayerTiles.begin();
			tile != this->lightedPlayerTiles.end(); ++tile)
	{
		const UINT wX = tile->wX, wY = tile->wY;
		const UINT wStartIndex = this->pRoom->ARRAYINDEX(wX,wY) * wLightValuesPerTile;
		LIGHTTYPE *pSrc, *pDest = this->lightMaps.psDisplayedLight + wStartIndex;

		//Blur this light if it would smooth some shadow edges.
		if (this->partialLightedTiles.has(wX,wY))
		{
			//Whether there is light at the edges of the adjacent tiles to factor in.
			const bool bUp = wY > 0 && this->tileLightInfo.GetAt(wX,wY-1) != L_Dark;
			const bool bDown = wY < wLastRow && this->tileLightInfo.GetAt(wX,wY+1) != L_Dark;
			const bool bLeft = wX > 0 && this->tileLightInfo.GetAt(wX-1,wY) != L_Dark;
			const bool bRight = wX < wLastCol && this->tileLightInfo.GetAt(wX+1,wY) != L_Dark;

			LowPassLightFilter(this->lightMaps.psPlayerLight + wStartIndex, this->lightMaps.psTempLightBuffer + wStartIndex,
					bLeft, bRight, bUp, bDown);

			//Add the blurred light.
			pSrc = this->lightMaps.psTempLightBuffer + wStartIndex;
		} else {
			//Add light unchanged from the original buffer.
			pSrc = this->lightMaps.psPlayerLight + wStartIndex;
		}

		const LIGHTTYPE *pSrcTileEnd = pSrc + wLightValuesPerTile;
		while (pSrc != pSrcTileEnd)
		{
			//RGB
			UINT val = *pDest + *(pSrc++);
			*(pDest++) = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1); //avoid overflow
			val = *pDest + *(pSrc++);
			*(pDest++) = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1);
			val = *pDest + *(pSrc++);
			*(pDest++) = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1);
		}
	}
}

//*****************************************************************************
// Calculates lighting from player's light source onto the respective data structures,
// for subsequent display in the room.
//
// Pre-condition: player light vars are clear
void CRoomWidget::PropagatePlayerLight()
{
	//Light params.
	if (this->pCurrentGame->playerLightType < 0) //no light
		return;

	this->pActiveLightedTiles = &this->lightedPlayerTiles;
	this->lightMaps.pActiveLight = this->lightMaps.psPlayerLight;

	PropagatePlayerLikeEntityLight(GetLightholder());

	if (IsTemporalCloneLightRendered())
	{
		for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		{
			if (pMonster->wProcessSequence > SPD_TEMPORALCLONE)
			{
				break;
			}

			if (pMonster->wType != M_TEMPORALCLONE)
			{
				continue;
			}

			PropagatePlayerLikeEntityLight(DYN_CAST(CEntity*, CMonster*, pMonster));
		}
	}
	
}


//*****************************************************************************
// Determine a cartesian coordinate position for an entity light source and cast it.
void CRoomWidget::PropagatePlayerLikeEntityLight(CEntity *pEntity)
{
	//Light params.
	if (this->pCurrentGame->playerLightType < 0 || pEntity == NULL) //no light
		return;

	const bool bSpotlight = this->pCurrentGame->playerLightType > 0;
	const int customPlayerLightParam = this->pCurrentGame->playerLight;

	//Position and direction, if applicable.
	float fxPos = float(pEntity->wX), fyPos = float(pEntity->wY);
	float fZ = getTileElev(this->pRoom->GetOSquare(pEntity->wX, pEntity->wY));
	float fSpotlightDirX = float(nGetOX(pEntity->wO));
	float fSpotlightDirY = float(nGetOY(pEntity->wO));
	if (this->dwMovementStepsLeft && ShowShadowCastingAnimation())
	{
		//interpolate between previous and current locations and tile heights
		const float animation_coeff = 1.0f - (this->dwCurrentDuration / float(this->dwMoveDuration));
		fxPos += int(pEntity->wPrevX - pEntity->wX) * animation_coeff;
		fyPos += int(pEntity->wPrevY - pEntity->wY) * animation_coeff;
		fZ += (getTileElev(this->pRoom->GetOSquare(pEntity->wPrevX, pEntity->wPrevY)) - fZ) * animation_coeff;

		if (bSpotlight && pEntity->wO != pEntity->wPrevO) {
			fSpotlightDirX += float(nGetOX(pEntity->wPrevO)) * animation_coeff;
			fSpotlightDirY += float(nGetOY(pEntity->wPrevO)) * animation_coeff;
		}
	}

	//Properties of player light.
	UINT wLightParam;
	if (!customPlayerLightParam) {
		static const UINT wDefaultPlayerLightRadius = 4;
		static const UINT wDefaultPlayerLightColor = 0; //white
		wLightParam = (wDefaultPlayerLightRadius - 1)*NUM_LIGHT_TYPES + wDefaultPlayerLightColor;
	}
	else
	{
		wLightParam = (customPlayerLightParam - 1) % LIGHT_OFF;
	}

	if (bSpotlight) {
		Point direction(fSpotlightDirX, fSpotlightDirY, 0.0f);
		direction.normalize();
		direction.z() = -0.1f; //slightly downward
		PropagateLight(fxPos, fyPos, fZ, wLightParam, true, direction);
	}
	else
	{
		PropagateLight(fxPos, fyPos, fZ, wLightParam);
	}

	if (pEntity == &this->pCurrentGame->swordsman)
	{
		this->wLastPlayerLightX = pEntity->wX;
		this->wLastPlayerLightY = pEntity->wY;
	}
}

//*****************************************************************************
void CRoomWidget::ResetPlayerLightMap()
{
	for (CCoordSet::const_iterator tile = this->lightedPlayerTiles.begin();
			tile != this->lightedPlayerTiles.end(); ++tile)
	{
		const UINT wStartIndex = this->pRoom->ARRAYINDEX(tile->wX,tile->wY) * wLightValuesPerTile;
		memset(this->lightMaps.psPlayerLight + wStartIndex, 0, wLightBytesPerTile);
		this->pTileImages[this->pRoom->ARRAYINDEX(tile->wX,tile->wY)].dirty = 1;
	}
	this->lightedPlayerTiles.clear();
}

//*****************************************************************************
void CRoomWidget::AddLightInterp(
//Add light color+intensity to this room tile.
//
//Params:
	SDL_Surface *pDestSurface,		//(in) dest surface
	const UINT wX, const UINT wY,	//(in) room position (in tiles)
	LIGHTTYPE* sRGBIntensity,	//(in) what % of light to add
	const float fDark,      //(in) darkness multiplier also being added to tile
	const UINT wTileMask,	//(in) optional tile mask [default = TI_UNSPECIFIED]
	const Uint8 opacity,    //adding light to a transparent object? [default=255]
	const int yRaised,     //vertical offset [default=0]
	SDL_Rect* crop) //if not NULL, defines a sub-tile crop area
const
{
	//Crop when altering out-of-bounds area.
	if (yRaised < 0 && wY == 0 && !crop) {
		static SDL_Rect local_crop = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
		local_crop.y = -yRaised;
		local_crop.h = CY_TILE + yRaised;
		crop = &local_crop;
	}

	//origin of tile blit on the surface
	const int xPixel = this->x + wX * CX_TILE;
	const int yPixel = this->y + wY * CY_TILE + yRaised;

	//Determine which sides of cropping area are being clipped by being flush against the room boundaries.
	//Set values to 1 to indicate that side was clipped, otherwise 0.
	SDL_Rect atRoomEdge = { 0, 0, 0, 0 };
	if (crop) {
		atRoomEdge.x = (xPixel + crop->x <= this->x) ? 1 : 0;
		atRoomEdge.y = (yPixel + crop->y <= this->y) ? 1 : 0;
		atRoomEdge.w = (xPixel + crop->w >= this->x + int(this->w)) ? 1 : 0;
		atRoomEdge.h = (yPixel + crop->h >= this->y + int(this->h)) ? 1 : 0;
	}

	const float dark_value = fDark / 256.0f; //div by 256 instead of 255 for speed optimization
	const bool add_darkness = fDark < 1.0f;

	//The below 2-D loop iterates over the sub-sampled tile light and shadow values.
	//Apply clipping to the loop range where necessary.
	UINT xClip, yClip, wClip, hClip;
	UINT iStart, jStart, iEnd, jEnd;
	CropAddLightParams(crop, atRoomEdge, xClip, yClip, wClip, hClip, iStart, jStart, iEnd, jEnd, sRGBIntensity);

	if (!(this->pRoom->tileLights.Exists(wX,wY) ||
			this->lightedRoomTiles.has(wX,wY) || this->lightedPlayerTiles.has(wX,wY)))
	{
		//This tile wasn't lighted by anything -- only add darkness.
		if (fDark < 1.0)
		{
			if (wTileMask == TI_UNSPECIFIED) {
				g_pTheDBM->DarkenRect(xPixel + xClip, yPixel + yClip, wClip, hClip, fDark, pDestSurface);
			} else {
				g_pTheDBM->DarkenTileWithMask(wTileMask, xClip, yClip,
					xPixel + xClip, yPixel + yClip, wClip, hClip, pDestSurface, fDark);
			}
		}
		return;
	}

	const UINT wMax = 3 * (UINT(fMaxLightIntensity[this->wDark] * opacity) + 256); //optimization

	UINT wXPixel, wYPixel;
	UINT wR[4], wG[4], wB[4];
	UINT wXOffset, wYOffset, wWidth, wHeight; //relative coordinates of a sub-tile light cell within the tile, i.e., values in [0,CX_TILE], etc.

	UINT i, j, k;	//sub-tile lighting
	for (j = jStart; j < jEnd; ++j, sRGBIntensity += LIGHT_BPP * (
		(LIGHT_SPT - iEnd) + //advance pointer past rest of this row
		iStart) //and also advance to where to start on the next row
		)
	{
		wYOffset = wLightCellPos[j];
		wHeight = wLightCellSize[j];

		//Clipping
		if (crop) {
			if (wYOffset < yClip) {
				const UINT delta = yClip - wYOffset;
				if (delta >= wHeight)
					continue; //nothing remains of this sub-tile row after clipping
				wHeight -= delta;
				wYOffset = yClip;
			}
			if (wYOffset + wHeight > yClip + hClip) {
				const UINT delta = wYOffset + wHeight - (yClip + hClip);
				if (delta >= wHeight)
					continue;
				wHeight -= delta;
			}
		}

		wYPixel = yPixel + wYOffset;

		for (i = iStart; i < iEnd; ++i, sRGBIntensity += LIGHT_BPP) //advance to next sub-tile RGB value
		{
			wXOffset = wLightCellPos[i];
			wWidth = wLightCellSize[i];

			//Clipping
			if (crop) {
				if (wXOffset < xClip) {
					const UINT delta = xClip - wXOffset;
					if (delta >= wWidth)
						continue; //nothing remains of this sub-tile cell after clipping
					wWidth -= delta;
					wXOffset = xClip;
				}
				if (wXOffset + wWidth > xClip + wClip) {
					const UINT delta = wXOffset + wWidth - (xClip + wClip);
					if (delta >= wWidth)
						continue;
					wWidth -= delta;
				}
			}

			wXPixel = xPixel + wXOffset;

			const LIGHTTYPE *pLight = sRGBIntensity;
			UINT wSum = wR[0] = *(pLight++);
			wSum += wG[0] = *(pLight++);
			wSum += wB[0] = *(pLight++);

			wSum += wR[1] = *(pLight++);
			wSum += wG[1] = *(pLight++);
			wSum += wB[1] = *pLight;

			pLight = sRGBIntensity + wLightValuesPerTileRow;
			wSum += wR[2] = *(pLight++);
			wSum += wG[2] = *(pLight++);
			wSum += wB[2] = *(pLight++);

			wSum += wR[3] = *(pLight++);
			wSum += wG[3] = *(pLight++);
			wSum += wB[3] = *pLight;

			if (!wSum)
			{
				//There is no light to add here.
				//If darkness is set, add that.
				if (fDark < 1.0)
				{
					if (wTileMask == TI_UNSPECIFIED)
						g_pTheDBM->DarkenRect(wXPixel, wYPixel,
								wWidth, wHeight, fDark, pDestSurface);
					else
						g_pTheDBM->DarkenTileWithMask(wTileMask, wXOffset, wYOffset,
								wXPixel, wYPixel,
								wWidth, wHeight, pDestSurface, fDark);
				}

				continue;
			}
			for (k=4; k--; )
			{
				//add 100% to the amount of multiplicative light so the math works correctly in LightenRect
				wR[k] += 256;
				wG[k] += 256;
				wB[k] += 256;

				//Reduce light by darkness factor.
				wR[k] = static_cast<UINT>(wR[k] * fDark);
				wG[k] = static_cast<UINT>(wG[k] * fDark);
				wB[k] = static_cast<UINT>(wB[k] * fDark);

				//Cap values at maximum light addition.  Don't alter hue or saturation.
				wSum = wR[k] + wB[k] + wG[k];
				if (wSum > wMax)
				{
					const float fNormalize = wMax / float(wSum);
					wR[k] = static_cast<UINT>(wR[k] * fNormalize);
					wG[k] = static_cast<UINT>(wG[k] * fNormalize);
					wB[k] = static_cast<UINT>(wB[k] * fNormalize);
				}
			}

			g_pTheBM->LightenRect(pDestSurface, wXPixel, wYPixel,
					wWidth, wHeight,
					wR, wG, wB,
					wTileMask, wXOffset, wYOffset);
		}
	}
}

//*****************************************************************************
void CRoomWidget::AddLightOffset(
//Add light color+intensity to a given moving tile with a sprite mask set.
//Calculates dark and light based on the possible 4 tiles the moving tile
//may be straddling.
//
//Use this to add lighting onto a moving sprite.
//Darkness value is mixed together with the light for higher end resolution.
//
//Params:
	SDL_Surface *pDestSurface,		//(in) dest surface
	const TileImageBlitParams& blit)
const
{
	//Must have an offset, otherwise we would've been better off using AddLight
	ASSERT(blit.wXOffset || blit.wYOffset);

	const UINT yRaised = blit.bDrawRaised ? -int(CY_RAISED) : 0;

	const UINT xIndex = blit.wCol * CX_TILE + blit.wXOffset; //blit position relative to room origin
	const UINT yIndex = blit.wRow * CY_TILE + blit.wYOffset;
	const UINT wXPos = this->x + xIndex;
	const UINT wYPos = this->y + yIndex + yRaised;

	const UINT wOX    = xIndex / CX_TILE;
	const UINT wOY    = yIndex / CY_TILE;
	const UINT wOXOff = xIndex % CX_TILE;
	const UINT wOYOff = yIndex % CY_TILE;

	const float fMax = 3 * (1.0f + (fMaxLightIntensity[this->wDark] * blit.nOpacity / 255)); //optimization
	UINT wNX, wNY, wNXPos, wNYPos, w, h;
	const UINT wOffsetWidth  = CX_TILE - wOXOff;
	const UINT wOffsetHeight = CY_TILE - wOYOff;
	for (int nQuadrant = 0; nQuadrant < 4; ++nQuadrant)
	{
		switch(nQuadrant)
		{
			case 0:  //NW Quadrant
				wNX = wOX;     wNXPos = 0;             w = wOffsetWidth;
				wNY = wOY;     wNYPos = 0;             h = wOffsetHeight;
				break;
			case 1:  //NE Quadrant
				wNX = wOX + 1; wNXPos = wOffsetWidth;  w = wOXOff;
				wNY = wOY;     wNYPos = 0;             h = wOffsetHeight;
				break;
			case 2:  //SW Quadrant
				wNX = wOX;     wNXPos = 0;             w = wOffsetWidth;
				wNY = wOY + 1; wNYPos = wOffsetHeight; h = wOYOff;
				break;
			case 3:  //SE Quadrant
				wNX = wOX + 1; wNXPos = wOffsetWidth;  w = wOXOff;
				wNY = wOY + 1; wNYPos = wOffsetHeight; h = wOYOff;
				break;
		}
		if (!w || !h) continue;
  		if (!this->pRoom->IsValidColRow(wNX, wNY)) continue;

		//Calculate dark
		const float fDark = GetOverheadDarknessAt(wNX, wNY, blit.appliedDarkness);
		const float dark_value = fDark/256.0f; //div by 256 instead of 255 for speed optimization
		const bool add_darkness = fDark < 1.0f;

		//Add light to sprite
		if (this->pRoom->tileLights.Exists(wNX,wNY) ||
			 this->lightedRoomTiles.has(wNX,wNY) || this->lightedPlayerTiles.has(wNX,wNY))
		{
//			wXPos/wYPos is where we're drawing the tile.
//          wNX/wNY is what we're using to calculate light
//          wNXPos/wNYPos is the offset of wXPos/wYPos for us to draw.
//          w/h is the width and height of this quadrant

			float R, G, B;
			UINT wR, wG, wB;
			UINT wLX, wLY, wLW, wLH, wSum;
			UINT i,j,iMin,jMin,iMax,jMax;	//sub-tile lighting
			switch(nQuadrant)
			{
				case 0:  //NW Quadrant
					iMax = LIGHT_SPT_MINUSONE;
					iMin = iMax - 1;
					while(wLightCellPos[iMin] > CX_TILE - w) iMin--;
					jMax = LIGHT_SPT_MINUSONE;
					jMin = jMax - 1;
					while(wLightCellPos[jMin] > CY_TILE - h) jMin--;
					break;
				case 1:  //NE Quadrant
					iMin = 0;
					iMax = iMin + 1;
					while((iMax < LIGHT_SPT_MINUSONE) && (wLightCellPos[iMax] < w)) iMax++;
					jMax = LIGHT_SPT_MINUSONE;
					jMin = jMax - 1;
					while(wLightCellPos[jMin] > CY_TILE - h) jMin--;
					break;
				case 2:  //SW Quadrant
					iMax = LIGHT_SPT_MINUSONE;
					iMin = iMax - 1;
					while(wLightCellPos[iMin] > CX_TILE - w) iMin--;
					jMin = 0;
					jMax = jMin + 1;
					while((jMax < LIGHT_SPT_MINUSONE) && (wLightCellPos[jMax] < h)) jMax++;
					break;
				case 3:  //SE Quadrant
					iMin = 0;
					iMax = iMin + 1;
					while((iMax < LIGHT_SPT_MINUSONE) && (wLightCellPos[iMax] < w)) iMax++;
					jMin = 0;
					jMax = jMin + 1;
					while((jMax < LIGHT_SPT_MINUSONE) && (wLightCellPos[jMax] < h)) jMax++;
					break;
			}
			const LIGHTTYPE *sRGBIntensity = this->lightMaps.psDisplayedLight
				+ (wNY*this->pRoom->wRoomCols + wNX)*wLightValuesPerTile
				+ jMin*LIGHT_SPT*LIGHT_BPP;

			const bool west_quadrants = !(nQuadrant % 2);
			const bool north_quadrants = nQuadrant < 2;

			const UINT rowEndOffset = (LIGHT_SPT-iMax)*LIGHT_BPP;
			for (j = jMin; j<jMax; ++j)
			{
				sRGBIntensity += iMin*LIGHT_BPP; //start row
				for (i = iMin; i<iMax; ++i)
				{
					//Define where and how much of the tile we'll be drawing
					wLX = wLightCellPos[i];
					wLY = wLightCellPos[j];
					wLW = wLightCellSize[i];
					wLH = wLightCellSize[j];
					// Western Quadrants
					if (west_quadrants)
					{
						if (i == iMin)
						{
							wLX = 0;
							wLW -= CX_TILE - w - wLightCellPos[i];
						} else {
							wLX -= CX_TILE - w;
						}
					} else {
						// Eastern Quadrants
						if (i+1 == iMax) wLW = w - wLX;
						wLX += wNXPos;
					}

					// Northern Quadrants
					if (north_quadrants)
					{
						if (j == jMin)
						{
							wLY = 0;
							wLH -= CY_TILE - h - wLightCellPos[j];
						} else {
							wLY -= CY_TILE - h;
						}
					} else {
						// Southern Quadrants
						if (j+1 == jMax) wLH = h - wLY;
						wLY += wNYPos;
					}

					//Average values of four corners for lighting this sub-tile rectangle.
					wSum = wR = (sRGBIntensity[0] + sRGBIntensity[3] +
							sRGBIntensity[0+wLightValuesPerTileRow] + sRGBIntensity[3+wLightValuesPerTileRow]) / 4;
					wSum += wG = (sRGBIntensity[1] + sRGBIntensity[4] +
							sRGBIntensity[1+wLightValuesPerTileRow] + sRGBIntensity[4+wLightValuesPerTileRow]) / 4;
					wSum += wB = (sRGBIntensity[2] + sRGBIntensity[5] +
							sRGBIntensity[2+wLightValuesPerTileRow] + sRGBIntensity[5+wLightValuesPerTileRow]) / 4;
					if (!wSum)
					{
						//There is no light to add here.
						//If darkness is set, add that.
						if (add_darkness)
						{
							if (blit.wTileImageNo == TI_UNSPECIFIED)
								g_pTheDBM->DarkenRect(wXPos + wLX, wYPos + wLY,	wLW, wLH, fDark, pDestSurface);
							else
								g_pTheDBM->DarkenTileWithMask(blit.wTileImageNo, wLX, wLY,
										wXPos + wLX, wYPos + wLY, wLW, wLH, pDestSurface, fDark);
						}

						sRGBIntensity += LIGHT_BPP;
						continue;
					}

					//add 256 (unity) so math for multiplicative light blending works correctly
					R = (wR+256.0f) * dark_value;
					G = (wG+256.0f) * dark_value;
					B = (wB+256.0f) * dark_value;

					//Cap values at maximum light addition.  Don't alter hue or saturation.
					const float fSum = R + G + B;
					if (fSum > fMax)
					{
						const float fNormalize = fMax / fSum;
						R *= fNormalize;
						G *= fNormalize;
						B *= fNormalize;
					}

					g_pTheBM->LightenRectWithTileMask(pDestSurface, wXPos + wLX, wYPos + wLY, wLW, wLH,
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
							B, G, R,
#else
							R, G, B,
#endif
							blit.wTileImageNo, wLX, wLY);

					sRGBIntensity += LIGHT_BPP; //get next sub-tile RGB value
				}
				sRGBIntensity += rowEndOffset; //finish row
			}
		}
	}
}


//*****************************************************************************
void CRoomWidget::AddLight(
//Add light color+intensity to a given screen position (within the room)
//with a sprite mask set.
//
//Use this to add lighting onto the sprite.
//Darkness value is mixed together with the light for higher end resolution.
//
//Params:
	SDL_Surface *pDestSurface,		//(in) dest surface
	const UINT wXPos, const UINT wYPos,	//(in) pixel position on surface to apply tile lighting at
	LIGHTTYPE* sRGBIntensity,	//(in) what % of light to add
	const float fDark,      //(in) darkness multiplier also being added to tile
	const UINT wTileMask,	//(in) tile mask
	const Uint8 opacity,    //adding light to a transparent object? [default=255]
	SDL_Rect* crop) //if not NULL, defines a relative sub-tile crop region [default=NULL]
const
{
	if (!CropTileBlitToRoomBounds(crop, wXPos, wYPos))
		return;

	//Determine which sides of cropping area are being clipped by being flush against the room boundaries.
	//Set values to 1 to indicate that side was clipped, otherwise 0.
	SDL_Rect atRoomEdge = { 0, 0, 0, 0 };
	if (crop) {
		atRoomEdge.x = (int(wXPos) + crop->x <= this->x) ? 1 : 0;
		atRoomEdge.y = (int(wYPos) + crop->y <= this->y) ? 1 : 0;
		atRoomEdge.w = (wXPos + crop->w >= this->x + this->w) ? 1 : 0;
		atRoomEdge.h = (wYPos + crop->h >= this->y + this->h) ? 1 : 0;
	}

	const float dark_value = fDark/256.0f; //div by 256 instead of 255 for speed optimization
	const bool add_darkness = fDark < 1.0f;

	//The below 2-D loop iterates over the sub-sampled tile light and shadow values.
	//Apply clipping to the loop range where necessary.
	UINT xClip, yClip, wClip, hClip;
	UINT iStart, jStart, iEnd, jEnd;
	CropAddLightParams(crop, atRoomEdge, xClip, yClip, wClip, hClip, iStart, jStart, iEnd, jEnd, sRGBIntensity);

	UINT wXOffset, wYOffset, wWidth, wHeight; //relative coordinates of a sub-tile light cell within the tile, i.e., values in [0,CX_TILE], etc.
	UINT wX, wY; //pixel location to draw a sub-tile light cell

	const float fMax = 3 * (1.0f + (fMaxLightIntensity[this->wDark] * opacity / 255)); //optimization
	float R, G, B;
	UINT wR, wG, wB;

	//Interpolate (i.e., average the corners) lighting for each sub-tile light cell.
	//Apply uniform light and shadow value to the pixels in the cell.
	UINT i,j;	//sub-tile lighting
	for (j = jStart; j < jEnd; ++j, sRGBIntensity += LIGHT_BPP * (
		(LIGHT_SPT - iEnd) + //advance pointer past rest of this row
		iStart) //and also advance to where to start on the next row
		)
	{
		wYOffset = wLightCellPos[j];
		wHeight = wLightCellSize[j];

		//Clipping
		if (crop) {
			if (wYOffset < yClip) {
				const UINT delta = yClip - wYOffset;
				if (delta >= wHeight)
					continue; //nothing remains of this sub-tile row after clipping
				wHeight -= delta;
				wYOffset = yClip;
			}
			if (wYOffset + wHeight > yClip + hClip) {
				const UINT delta = wYOffset + wHeight - (yClip + hClip);
				if (delta >= wHeight)
					continue;
				wHeight -= delta;
			}
		}

		wY = wYPos + wYOffset;

		for (i = iStart; i < iEnd; ++i, sRGBIntensity += LIGHT_BPP) //advance to next sub-tile RGB value
		{
			wXOffset = wLightCellPos[i];
			wWidth = wLightCellSize[i];

			//Clipping
			if (crop) {
				if (wXOffset < xClip) {
					const UINT delta = xClip - wXOffset;
					if (delta >= wWidth)
						continue; //nothing remains of this sub-tile cell after clipping
					wWidth -= delta;
					wXOffset = xClip;
			}
				if (wXOffset + wWidth > xClip + wClip) {
					const UINT delta = wXOffset + wWidth - (xClip + wClip);
					if (delta >= wWidth)
						continue;
					wWidth -= delta;
				}
		}

			wX = wXPos + wXOffset;

			//Average values of four corners for lighting this sub-tile rectangle.
			UINT wSum = wR = (sRGBIntensity[0] + sRGBIntensity[3] +
				sRGBIntensity[0 + wLightValuesPerTileRow] + sRGBIntensity[3 + wLightValuesPerTileRow]) / 4;
			wSum += wG = (sRGBIntensity[1] + sRGBIntensity[4] +
				sRGBIntensity[1 + wLightValuesPerTileRow] + sRGBIntensity[4 + wLightValuesPerTileRow]) / 4;
			wSum += wB = (sRGBIntensity[2] + sRGBIntensity[5] +
				sRGBIntensity[2 + wLightValuesPerTileRow] + sRGBIntensity[5 + wLightValuesPerTileRow]) / 4;
			if (!wSum)
			{
				//There is no light to add here.
				//If darkness is set, add that.
				if (add_darkness)
				{
					if (wTileMask == TI_UNSPECIFIED)
						g_pTheDBM->DarkenRect(wX, wY,
							wWidth, wHeight, fDark, pDestSurface);
					else
						g_pTheDBM->DarkenTileWithMask(wTileMask, wXOffset, wYOffset,
							wX, wY,
							wWidth, wHeight, pDestSurface, fDark);
				}

				continue;
			}

			//add 256 (unity) so math for multiplicative light blending works correctly
			R = (wR + 256.0f) * dark_value;
			G = (wG + 256.0f) * dark_value;
			B = (wB + 256.0f) * dark_value;

			//Cap values at maximum light addition.  Don't alter hue or saturation.
			const float fSum = R + G + B;
			if (fSum > fMax)
			{
				const float fNormalize = fMax / fSum;
				R *= fNormalize;
				G *= fNormalize;
				B *= fNormalize;
			}

			g_pTheBM->LightenRectWithTileMask(pDestSurface, wX, wY,
				wWidth, wHeight,
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
				B, G, R,
#else
				R, G, B,
#endif
				wTileMask, wXOffset, wYOffset);
	}
}
}

//*****************************************************************************
//Outputs a relative region within a tile to apply lighting.
//By default, this is the entire tile area.
//When a cropping region is provided, apply it to restrict the region to be lighted.
void CRoomWidget::CropAddLightParams(
	const SDL_Rect* crop, //If not NULL, indicates the rectangular region of the tile to apply lighting to
	const SDL_Rect& roomEdgeClip, //non-zero values indicates 'crop' was clipped against that side of the room
	UINT& x, UINT& y, UINT& w, UINT& h, //(out)
	UINT& iStart, UINT& jStart, UINT& iEnd, UINT& jEnd, //(out)
	LIGHTTYPE*& sRGBIntensity) //(out) relative offset for the starting address of sub-tile light map memory to apply
const
{
	//Sub-tile light cell range to apply lighting from.
	//Defaults to entire tile.
	iStart = jStart = 0;
	iEnd = LIGHT_SPT_MINUSONE;
	jEnd = LIGHT_SPT_MINUSONE;

	if (!crop) {
		//Apply lighting to full tile.
		x = y = 0;
		w = CX_TILE;
		h = CY_TILE;
		return;
	}

	x = crop->x;
	y = crop->y;
	w = crop->w;
	h = crop->h;

	ASSERT(x < UINT(CX_TILE));
	ASSERT(y < UINT(CY_TILE));
	ASSERT(x + w <= UINT(CX_TILE));
	ASSERT(y + h <= UINT(CY_TILE));

	//Within the room, start at light cell containing (x,y) and end at (x+w,y+h).
	while (iStart < iEnd - 1 && wLightCellPos[iStart + 1] <= x)
		++iStart;
	while (jStart < jEnd - 1 && wLightCellPos[jStart + 1] <= y)
		++jStart;
	while (iEnd > iStart + 1 && wLightCellPos[iEnd - 1] > x + w)
		--iEnd;
	while (jEnd > jStart + 1 && wLightCellPos[jEnd - 1] > y + h)
		--jEnd;

	//However, when clipping to room boundaries, the lighting at the room's edge
	//needs to be applied rather than the interior lighting of the tile.
	//To do this, for example, when x is clipped (i.e., increased)
	//due to the blit being slightly outside the left side of the room:
	//rather than reading from the lightmap at iStart (which is often non-zero
	//in that case, and thereby would apply the lighting from the right side of
	//the room's leftmost tile, which would look wrong), apply the lighting
	//at the left edge of the tile, i.e., where the room edge is.

	if (!roomEdgeClip.y) {
		if (!roomEdgeClip.h) {
			sRGBIntensity += jStart * wLightValuesPerTileRow; //standard in-room case
		}
		else {
			sRGBIntensity += (LIGHT_SPT_MINUSONE - jEnd) * wLightValuesPerTileRow; //use lighting at bottom of tile
		}
	} //else: use lighting at top of tile

	if (!roomEdgeClip.x) {
		if (!roomEdgeClip.w) {
			sRGBIntensity += iStart * LIGHT_BPP; //standard in-room case
		}
		else {
			sRGBIntensity += (LIGHT_SPT_MINUSONE - iEnd) * LIGHT_BPP; //use lighting on right side of tile
		}
	} //else: use lighting on left side of tile
}

//*****************************************************************************
//Provides a clipping rectangle if needed, and one was not provided
bool CRoomWidget::CropTileBlitToRoomBounds(SDL_Rect*& crop, int dest_x, int dest_y) const
{
	if (dest_x < this->x ||
		dest_y < this->y ||
		dest_x + int(CX_TILE) >(this->x + int(this->w)) ||
		dest_y + int(CY_TILE) >(this->y + int(this->h)))
	{
		//Blitting a tile at indicated location would not be entirely within room -- crop to room region.
		if (!crop) {
			static SDL_Rect local_crop;
			local_crop.x = local_crop.y = 0;
			local_crop.w = CX_TILE;
			local_crop.h = CY_TILE;
			crop = &local_crop;
		}

		return ClipTileArea(dest_x, dest_y, *crop);
	}

	return true;
}

//*****************************************************************************
float CRoomWidget::GetOverheadDarknessAt(const UINT wX, const UINT wY,
	const float fIntensity) //[default=1.0] input a value <1 to reduce the amount of darkening applied
const
{
	const UINT darkVal = this->pRoom->tileLights.GetAt(wX, wY);
	if (bIsDarkTileValue(darkVal))
	{
		ASSERT(darkVal - LIGHT_OFF - 1 < NUM_DARK_TYPES);
		ASSERT(0.0f <= fIntensity && fIntensity <= 1.0f);
		return darkMap[UINT((darkVal - LIGHT_OFF - 1) * fIntensity)];
	}
	return 1.0f;
}

//*****************************************************************************
void CRoomWidget::ApplyDisplayFilter(int displayFilter, SDL_Surface* pDestSurface, UINT wX, UINT wY)
{
	switch (displayFilter) {
		default:
		case ScriptFlag::D_Normal: break;
		case ScriptFlag::D_BandW: BandWTile(pDestSurface, wX, wY); break;
		case ScriptFlag::D_Sepia: SepiaTile(pDestSurface, wX, wY); break;
		case ScriptFlag::D_Negative: NegativeTile(pDestSurface, wX, wY); break;
	}
}

//*****************************************************************************
void CRoomWidget::ApplyDisplayFilterToRoom(int displayFilter, SDL_Surface *pDestSurface)
{
	if (displayFilter == ScriptFlag::D_Normal)
		return;

	TileImages *pTI = this->pTileImages;
	const UINT rows = this->pRoom->wRoomRows;
	const UINT cols = this->pRoom->wRoomCols;
	UINT wX, wY;
	for (wY = 0; wY < rows; ++wY)
	{
		for (wX = 0; wX < cols; ++wX)
		{
			if (this->bAllDirty || pTI->damaged)
			{
				ApplyDisplayFilter(displayFilter, pDestSurface, wX, wY);
			}
			++pTI;
		}
	}
}

int CRoomWidget::getDisplayFilter() const
{
	if (this->displayFilterOverride != ScriptFlag::D_Nothing)
		return this->displayFilterOverride;
	return this->pCurrentGame ? this->pCurrentGame->displayFilter : ScriptFlag::D_Normal;
}

//*****************************************************************************
UINT CRoomWidget::GetTextureIndexForTile(const UINT tileNo, const bool bForceBaseImage) const
//Returns: the texture index that corresponds to this room tile
{
	UINT wTextureIndex = FLOOR_ROAD; //Prevent uninitialized
	switch (tileNo)
	{
		case T_FLOOR_ROAD: wTextureIndex = FLOOR_ROAD; break;
		case T_FLOOR_GRASS: wTextureIndex = FLOOR_GRASS; break;
		case T_FLOOR_DIRT: wTextureIndex = FLOOR_DIRT; break;
		case T_FLOOR_ALT: wTextureIndex = FLOOR_ALT; break;
		case T_FLOOR: wTextureIndex = FLOOR_CHECKERED; break;
		case T_FLOOR_M: wTextureIndex = FLOOR_MOSAIC; break;
		case T_PIT:
		case T_PLATFORM_P:
			wTextureIndex = PIT_MOSAIC;
			break;
		case T_WALL: case T_WALL_H: case T_WALL_B: case T_WALL2:
			wTextureIndex = WALL_MOSAIC;
			break;
		case T_FLOOR_IMAGE: wTextureIndex =
				!bForceBaseImage && g_pTheDBM->pTextures[FLOOR_IMAGE] ? FLOOR_IMAGE : FLOOR_MOSAIC;
			break;
		case T_PIT_IMAGE: wTextureIndex =
				!bForceBaseImage && g_pTheDBM->pTextures[FLOOR_IMAGE] ? FLOOR_IMAGE : PIT_MOSAIC;
			break;
		case T_WALL_IMAGE: wTextureIndex =
				!bForceBaseImage && g_pTheDBM->pTextures[FLOOR_IMAGE] ? FLOOR_IMAGE : WALL_MOSAIC;
			break;
		case T_OVERHEAD_IMAGE:
			wTextureIndex = g_pTheDBM->pTextures[OVERHEAD_IMAGE] ? OVERHEAD_IMAGE : WALL_MOSAIC;
			break;
		default:
			if (!bForceBaseImage)
			{
				wTextureIndex = FLOOR_MOSAIC;
				break;
			}
			ASSERT(!"Invalid tile for texturing");
		break;
	}
	return wTextureIndex;
}

//*****************************************************************************
void CRoomWidget::RenderRoom(
//Render o- and t-layers in room for either game play or the room editor.
//
//Params:
	int wCol, int wRow,     //(in) top-left tile coords [default=(0,0) to (xSize,ySize)]
	int wWidth, int wHeight,
	const bool bEditor)     //[default=true]
{
#define DrawRoomTile(wTileImageNo) g_pTheBM->BlitTileImage(\
		(wTileImageNo), nX, nY, pDestSurface, false, 255)
#define DrawTransparentRoomTile(wTileImageNo,opacity)\
		g_pTheBM->BlitTileImage((wTileImageNo), nX, nY, pDestSurface, false, (opacity))
#define AddDark(fLight) g_pTheDBM->DarkenTile(nX, nY, fLight, pDestSurface);
#define IsDeepWaterTile(wTile) ((wTile) == T_WATER || (wTile) == T_TRAPDOOR2 || (wTile) == T_PLATFORM_W || (wTile) == T_THINICE)
#define IsShallowTile(wTile) (bIsShallowWater((wTile)) || bIsSteppingStone((wTile)) || (wTile) == T_THINICE_SH)
#define IsShallowImage(wX,wY) (water[(wX)][(wY)] != T_WATER) &&\
		(water[(wX)][(wY)] == T_SHALLOW_WATER ||\
		(water[(wX)][(wY)] == T_FLOOR && (\
		water[(wX-1)][(wY)] == T_SHALLOW_WATER || water[(wX+1)][(wY)] == T_SHALLOW_WATER ||\
		water[(wX)][(wY-1)] == T_SHALLOW_WATER || water[(wX)][(wY+1)] == T_SHALLOW_WATER)))

	static const SURFACECOLOR SecretWallHighlightColor = { 128, 255, 255 };

	BoundsCheckRect(wCol,wRow,wWidth,wHeight);
	const UINT wRowOffset = this->pRoom->wRoomCols - wWidth;
	const UINT wLightRowOffset = wRowOffset * wLightValuesPerTile;
	const UINT wStartPos = wRow * this->pRoom->wRoomCols + wCol;
	const UINT wXEnd = wCol + wWidth;
	const UINT wYEnd = wRow + wHeight;

	//For pit rendering.
	SDL_Surface *pPitsideTexture = g_pTheDBM->pTextures[PITSIDE_MOSAIC];
	if (!pPitsideTexture)
		return;
	const UINT wPitHeight = static_cast<UINT>(pPitsideTexture->h);
	const UINT wPitSideTileWidth = pPitsideTexture->w / CX_TILE;

	SDL_Surface *pDestSurface = this->pRoomSnapshotSurface;  //draw to here

	SDL_Surface *pWaterSurface = NULL;  //For drawing Water/Shallow mixes
	SDL_Surface *pDeepBottom = g_pTheDBM->pTextures[DEEP_MOSAIC];
	SDL_Surface *pShallowBottom = g_pTheDBM->pTextures[SHALLOW_MOSAIC];

	TileImages *pTI = this->pTileImages + wStartPos;
	LIGHTTYPE *psL = this->lightMaps.psDisplayedLight + wStartPos * wLightValuesPerTile;

	const float fLightLevel = fRoomLightLevel[this->wDark];
	float fDark;
	const bool bAddLight = IsLightingRendered();

	bool bMosaicTile, bTransparentOTile, bTar, bNorthernWall,
			bTIsTransparent, bTIsTranslucent;
	bool bBlitCustomTextureTile = false;
	UINT wTextureIndex;
	UINT wTileNo, wOTileNo, wTTileNo;
	UINT wX, wY, wTI;
	int nX, nY;

	for (wY = wRow; wY < wYEnd; ++wY)
	{
		for (wX = wCol; wX < wXEnd; ++wX)
		{
			if (this->bAllDirty || pTI->dirty)
			{
				//Render one tile in the room by blitting successive sprites
				//on top of one another.
				nX = this->x + wX * CX_TILE;
				nY = this->y + wY * CY_TILE;
				wOTileNo = this->pRoom->GetOSquare(wX, wY);
				wTTileNo = this->pRoom->GetTSquare(wX, wY);
				wTI = pTI->o;
				bMosaicTile = bIsMosaic(wOTileNo) || wOTileNo == T_PLATFORM_P || //platforms have pit drawn under them
						wTI == TI_WALL_M; //draw inner wall tiles with mosaic
				//Northern wall tiles have mosaic drawn on the bottom half.
				if ((bNorthernWall = pTI->edges.bHalfWall))	//assign and check
					bMosaicTile = true;
				bTransparentOTile = wOTileNo == T_GOO || wOTileNo == T_FLOOR_IMAGE ||
						(wOTileNo == T_WALL_M && this->pCurrentGame && this->pCurrentGame->bHoldMastered) ||
						(wOTileNo == T_WALL_WIN && this->pCurrentGame && this->pCurrentGame->bHoldCompleted);
				bTar = bIsTarOrFluff(wTTileNo);
				bTIsTransparent = (bTar && bEditor);
				bTIsTranslucent = bTar && this->bRequestTranslucentTar;

				//Determine this tile's darkness.
				fDark = fLightLevel * GetOverheadDarknessAt(wX, wY);

				//1. Draw opaque (floor) layer.
				const bool bWater = bIsWater(wOTileNo) || wOTileNo == T_PLATFORM_W || bIsSteppingStone(wOTileNo) || bIsThinIce(wOTileNo);
				if (bWater)
					goto OLayerDone; //water is handled below
				if (!bMosaicTile && !bTransparentOTile)
				{
					DrawRoomTile(pTI->o);

					wTileNo = bTransparentOTile ?
						this->pRoom->coveredOSquares.GetAt(wX, wY) : wOTileNo;

					if (wOTileNo == T_WALL_B && this->puzzleModeOptions.GetShowBrokenWalls())
						ShadeRect(pDestSurface, SecretWallHighlightColor, wX, wY, 1, 1);
					else if (wOTileNo == T_WALL_H && this->puzzleModeOptions.GetShowSecretWalls())
							ShadeRect(pDestSurface, SecretWallHighlightColor, wX, wY, 1, 1);

					//In cases where doors are 2x2 tiles or thicker, draw filler to
					//remove the dimple on the edge.
					if (bIsDoor(wOTileNo))
						DrawDoorFiller(pDestSurface, wX,wY);

					//Special-case objects have extra effects rendered onto them.
					//Stairs: add special lighting.
					if (bIsStairs(wOTileNo))
					{
						UINT wColIgnored, wRow;
						if (wOTileNo == T_STAIRS)
						{
							CalcStairPosition(this->pRoom, wX, wY, wColIgnored, wRow);
							float fLight = 1.0f - (wRow-1)*0.04f; 
							if (fLight < 0.1) fLight = 0.1f;
							g_pTheDBM->DarkenTile(nX, nY, fLight, pDestSurface);
						} else {
							CalcStairUpPosition(this->pRoom, wX, wY, wColIgnored, wRow);
							float fLight = 1.0f + (wRow-1)*0.04f;
							if (fLight > 1.7) fLight = 1.7f;
							g_pTheDBM->LightenTile(pDestSurface, nX, nY, fLight);
						}
					}
				} else {
					//Draw tile from image texture.
					wTileNo = bTransparentOTile ?
							this->pRoom->coveredOSquares.GetAt(wX,wY) : wOTileNo;
					wTextureIndex = GetTextureIndexForTile(wTileNo, true); //base texture

					//Texture coords.
					SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
					SDL_Rect dest = MAKE_SDL_RECT(nX, nY, CX_TILE, CY_TILE);
					ASSERT(wTextureIndex < TEXTURE_COUNT);
					SDL_Surface *pTileTexture = g_pTheDBM->pTextures[wTextureIndex];
					static const int nHalfWallY = CY_TILE/2 +1;
					//Calculate coords, tiling texture from room origin.
					src.x = (wX * CX_TILE) % pTileTexture->w;
					src.y = (wY * CY_TILE) % pTileTexture->h;
					const bool bWallImage = wTileNo == T_WALL_IMAGE;
					if (bNorthernWall || bWallImage)
					{
						//Blit wall tile.
						if (bWallImage && GetWallTypeAtSquare(this->pRoom, wX, wY) != WALL_EDGE)
						{
							//Blit the base texture in case there are transparent areas.
							SDL_BlitSurface(pTileTexture, &src, pDestSurface, &dest);
						} else {
							DrawRoomTile(pTI->o);
						}

						if (bNorthernWall)
						{
							//Blit inner wall mosaic on bottom half of the tile only.
							src.y += nHalfWallY;
							src.h -= nHalfWallY;
							dest.y += nHalfWallY;
							dest.h -= nHalfWallY;
						}
					}

					//Blit the base texture.
					if (!bWallImage || bNorthernWall)
						SDL_BlitSurface(pTileTexture, &src, pDestSurface, &dest);

					//1a. Special textures are drawn on top of the standard ones.
					//This allows transparency in special image to show the normal texture underneath.
					if (wOTileNo == T_PIT_IMAGE && !g_pTheDBM->pTextures[FLOOR_IMAGE])
						wOTileNo = T_PIT; //revert to normal pit look
					if (wOTileNo == T_WALL_IMAGE && !g_pTheDBM->pTextures[FLOOR_IMAGE])
						wOTileNo = T_WALL; //revert to normal wall look
					wTextureIndex = GetTextureIndexForTile(wOTileNo, false);
					bBlitCustomTextureTile = wTextureIndex == FLOOR_IMAGE;

					//1b. Special-case objects with extra effects rendered onto them.
					switch (wOTileNo)
					{
					case T_FLOOR_IMAGE: break;
					case T_PIT:
					case T_PLATFORM_P:
						//Determine whether pit sides need to be drawn.
						//Only draw pit sides to bottom of pit side mosaic.
						if (pTI->edges.wPitY * CY_TILE < wPitHeight)
						{
							//If the pitside mosaic won't be plotted along its full width,
							//then use a smaller pitside texture for the remainder.
							UINT wPitX = pTI->edges.wPitX % wPitSideTileWidth;
							src.x = wPitX * CX_TILE;
							const bool bFullTexture = wPitX + pTI->edges.wPitRemaining >= wPitSideTileWidth;
							if (!bFullTexture)
								src.x = src.x % g_pTheDBM->pTextures[PITSIDE_SMALL]->w;

							//In order to show shadows under tall arches correctly,
							//must examine how far back the arch recedes.
							UINT wFloorDepth = 1;
							src.y = pTI->edges.wPitY * CY_TILE;
							while (src.y + CY_TILE < (int)wPitHeight) {src.y += CY_TILE; ++wFloorDepth;}

							//If floor is more than one square deep at pit edge,
							//draw successive layers of pitside to portray depth in transparent areas.
							do {
								ASSERT(src.y >= 0);
								const UINT wOSquareAbove = this->pRoom->GetOSquareWithGuessing(wX, wY - pTI->edges.wPitY - wFloorDepth);
								if (!bIsPit(wOSquareAbove) && wOSquareAbove != T_TRAPDOOR &&
										!bIsBridge(wOSquareAbove) && wOSquareAbove != T_PLATFORM_P)
								{
									//Draw partial pit side tile if there's a partial tile at the bottom of the source image.
									dest.h = (int)wPitHeight - src.y < CY_TILE ? wPitHeight - src.y : CY_TILE;

									SDL_BlitSurface(bFullTexture ?
											pPitsideTexture :
											g_pTheDBM->pTextures[PITSIDE_SMALL],
											&src, pDestSurface, &dest);
								}
								src.y -= CY_TILE;
							} while (--wFloorDepth);
						}

						//Draw side edge to trapdoors when hanging over pit.
						switch (this->pRoom->GetOSquareWithGuessing(wX, wY - 1))
						{
							case T_TRAPDOOR:
								DrawRoomTile(TI_TRAPDOOR_EDGE);
							break;
							case T_BRIDGE: case T_BRIDGE_H:
								DrawRoomTile(TI_BRIDGE_HEDGE);
							break;
							case T_BRIDGE_V:
								DrawRoomTile(TI_BRIDGE_VEDGE);
							break;
						}
					break;

					case T_WALL_H:
						//Show hidden broken walls in the editor.
						if (bEditor)
						{
							if (bNorthernWall)
								g_pTheBM->BlitTileImagePart(TI_WALL_H, nX, nY + nHalfWallY,
										0, nHalfWallY, CX_TILE, CY_TILE - nHalfWallY,
										pDestSurface, false, 196);
							else
								DrawTransparentRoomTile(TI_WALL_H, 196);
						} else if (this->puzzleModeOptions.GetShowSecretWalls())
							ShadeRect(pDestSurface, SecretWallHighlightColor, wX, wY, 1, 1);
					break;

					case T_WALL_B:
						if (this->puzzleModeOptions.GetShowBrokenWalls())
							ShadeRect(pDestSurface, SecretWallHighlightColor, wX, wY, 1, 1);
						break;

					case T_WALL_M:
					case T_WALL_WIN:
						//"Complete/Master" walls are transparent when hold is completed/mastered.
						if (bTransparentOTile)
							DrawTransparentRoomTile(pTI->o, 64);
					break;
					case T_GOO:
						ASSERT(bTransparentOTile);
						DrawTransparentRoomTile(pTI->o, 196);
						break;
					default:	ASSERT(!bTransparentOTile); break;
					}
				}
OLayerDone:
				//1c. Draw outline around squares that need it.
				DrawTileEdges(wX, wY, pTI, pDestSurface);

				//1b (cont.) -- Custom textures are blitted on top of wall edging.
				if (bBlitCustomTextureTile)
				{
					//Calculate coords for special floor texture, tiling from
					//indicated origin.
					SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
					SDL_Rect dest = MAKE_SDL_RECT(nX, nY, CX_TILE, CY_TILE);
					SDL_Surface *pTileTexture = g_pTheDBM->pTextures[wTextureIndex];
					UINT wSize = pTileTexture->w;
					int nI = ((int)wX-(int)this->pRoom->wImageStartX) * CX_TILE;
					while (nI < 0)
						nI += wSize;
					src.x = nI % wSize;
					wSize = pTileTexture->h;
					nI = ((int)wY-(int)this->pRoom->wImageStartY) * CY_TILE;
					while (nI < 0)
						nI += wSize;
					src.y = nI % wSize;

					SDL_BlitSurface(pTileTexture, &src, pDestSurface, &dest);
					bBlitCustomTextureTile = false;
				}

				//2a. Is there water or a water bank here?
				if (bWater || (bIsPlainFloor(wOTileNo) || wOTileNo == T_GOO))
				{
					bool bShallowWater = bIsShallowWater(wOTileNo) || bIsSteppingStone(wOTileNo) || wOTileNo == T_THINICE_SH;
					const UINT wWaterMask = bWater ? wOTileNo :
							CalcTileImageForWater(this->pRoom, wX, wY, T_WATER, &bShallowWater);

					if (bWater || wWaterMask != TI_WATER_NSWE)
					{
						const UINT wShallowTile = bShallowWater ?
								CalcTileImageForWater(this->pRoom, wX, wY, T_SHALLOW_WATER) :
								TI_SHALLOW_TOP;
						UINT wDeepMix = 0, wWSurfMasks = 0;

						if (!bShallowWater)
						{
							UINT water[5][5];
							for (UINT wTX = 0; wTX < 5; wTX++)
								for (UINT wTY = 0; wTY < 5; wTY++)
								{
									UINT wOSquare = T_WATER;
									if (this->pRoom->IsValidColRow(wTX+wX-2,wTY+wY-2))
									{
										wOSquare = this->pRoom->GetOSquare(wTX+wX-2,wTY+wY-2);
										if (IsDeepWaterTile(wOSquare)) {
											wOSquare = T_WATER;
										} else if (IsShallowTile(wOSquare)) {
											wOSquare = T_SHALLOW_WATER;
										} else if (bIsPlainFloor(wOSquare) || wOSquare == T_GOO) {
											wOSquare = T_FLOOR;
										}
									}

									water[wTX][wTY] = wOSquare;
								}

							const bool bNShallow = IsShallowImage(2,1);
							const bool bSShallow = IsShallowImage(2,3);
							const bool bWShallow = IsShallowImage(1,2);
							const bool bEShallow = IsShallowImage(3,2);
							if (bNShallow && bWShallow && water[1][1] != T_WATER)
								wDeepMix++;
							if (bNShallow && bEShallow && water[3][1] != T_WATER)
								wDeepMix += 2;
							if (bSShallow && bWShallow && water[1][3] != T_WATER)
								wDeepMix += 4;
							if (bSShallow && bEShallow && water[3][3] != T_WATER)
								wDeepMix += 8;
						}

						const bool bWaterMix = (bShallowWater && wShallowTile != TI_SHALLOW_TOP) || wDeepMix;
						if (bWaterMix)
						{
							if (!pWaterSurface)
								pWaterSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
										SDL_SWSURFACE, CX_TILE, 3*CY_TILE, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
							ASSERT(pWaterSurface);
							//pWaterSurface needs to create three things:
							//   1. The correct mixture of TI_WATER_TOP and TI_SHALLOW_TOP,
							//      using wShallowTile as a mask
							//   2. A Deep Water Mask using wShallowTile and wWaterMask
							//   3. A Shallow Water Mask using wShallowTile and wWaterMask
							wWSurfMasks = g_pTheDBM->CreateShallowWaterMix(bWater ? TI_WATER_TOP : wWaterMask,
									wShallowTile, wDeepMix, pWaterSurface);
						}

						SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
						SDL_Rect dest = MAKE_SDL_RECT(nX, nY, CX_TILE, CY_TILE);
						SDL_Rect deepRect = MAKE_SDL_RECT(0, CY_TILE, CX_TILE, CY_TILE);
						SDL_Rect shlwRect = MAKE_SDL_RECT(0, 2*CY_TILE, CX_TILE, CY_TILE);

						//2b. Draw bank and water.
						if (bWater)
						{
							if (bWaterMix)
								g_pTheBM->BlitSurface(pWaterSurface, &src, pDestSurface, &dest);
							else
								DrawRoomTile(bShallowWater ? TI_SHALLOW_TOP : TI_WATER_TOP);
						} else {
							DrawRoomTile(wWaterMask);
							if (bWaterMix)
								g_pTheBM->BlitWithTileMask(wWaterMask, src,
									pWaterSurface, dest, pDestSurface);
							else
								g_pTheBM->BlitTileWithTileMask(wWaterMask,
									bShallowWater ? TI_SHALLOW_TOP : TI_WATER_TOP, dest, pDestSurface);
						}

						//3. Show northern walls reflected in water,
						//   and side edges of trapdoors, etc.
						const UINT wOTileAbove = this->pRoom->GetOSquareWithGuessing(wX, wY-1);
						UINT wRefWallTile = TI_UNSPECIFIED;
						switch (wOTileAbove)
						{
							case T_WALL: case T_WALL2: wRefWallTile = TI_WALL_REF; break;
							case T_WALL_B: wRefWallTile = TI_WALL_BREF; break;
							case T_WALL_H: wRefWallTile = TI_WALL_HREF; break;
							case T_TRAPDOOR: case T_TRAPDOOR2:
								wRefWallTile = TI_TRAPDOOR_EDGE; break;
							case T_BRIDGE: case T_BRIDGE_H:
								wRefWallTile = TI_BRIDGE_HEDGE; break;
							case T_BRIDGE_V: wRefWallTile = TI_BRIDGE_VEDGE; break;
							default: break;
						}
						if (wRefWallTile != TI_UNSPECIFIED)
						{
							if (bWater)
							{
								DrawTransparentRoomTile(wRefWallTile, 128);
							}
							else
								g_pTheBM->BlitTileWithTileMask(wWaterMask, wRefWallTile,
										dest, pDestSurface, 128);
						}

						//4. Draw sky/water bottom.
						if (this->bOutside && this->pSkyImage)
						{
							const int wXOffset = this->dwSkyX % this->pSkyImage->w;
							src.x = (int)(wX * CX_TILE + wXOffset);
							src.y = wY * CY_TILE;
							if (bWaterMix)
							{
								if (wWSurfMasks & 0x01)
									g_pTheBM->BlitWithMask(deepRect, pWaterSurface, src,
											this->pSkyImage, dest, pDestSurface,
											SKY_DEEP_OPACITY);
								if (wWSurfMasks & 0x02)
								{
									g_pTheBM->BlitWithMask(shlwRect, pWaterSurface, src,
											this->pSkyImage, dest, pDestSurface,
											SKY_SHALLOW_OPACITY);
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitWithMask(shlwRect, pWaterSurface, src,
											pShallowBottom, dest,
											pDestSurface, SKY_BOTTOM_OPACITY);
								}
							} else if (bWater) {
								g_pTheBM->BlitWrappingSurface(this->pSkyImage, src,
										pDestSurface, dest,
										bShallowWater ? SKY_SHALLOW_OPACITY : SKY_DEEP_OPACITY);
								if (bShallowWater)
								{
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitSurface(pShallowBottom,
											&src, pDestSurface, &dest, SKY_BOTTOM_OPACITY);
								}
							} else {
								g_pTheBM->BlitWithTileMask(wWaterMask, src, this->pSkyImage,
										dest, pDestSurface,
										bShallowWater ? SKY_SHALLOW_OPACITY : SKY_DEEP_OPACITY);
								if (bShallowWater)
								{
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitWithTileMask(wWaterMask, src,
											pShallowBottom, dest,
											pDestSurface, SKY_BOTTOM_OPACITY);
								}
							}
						} else {
							//Calculate coords, tiling pit texture from room origin.
							src.x = (wX * CX_TILE) % pDeepBottom->w;
							src.y = (wY * CY_TILE) % pDeepBottom->h;
							if (bWaterMix)
							{
								if (wWSurfMasks & 0x01)
									g_pTheBM->BlitWithMask(deepRect, pWaterSurface, src,
											pDeepBottom, dest,
											pDestSurface, PIT_DEEP_OPACITY);
								if (wWSurfMasks & 0x02)
								{
									g_pTheBM->BlitWithMask(shlwRect, pWaterSurface, src,
											pDeepBottom, dest,
											pDestSurface, PIT_SHALLOW_OPACITY);
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitWithMask(shlwRect, pWaterSurface, src,
											pShallowBottom, dest,
											pDestSurface, PIT_BOTTOM_OPACITY);
								}
							} else if (bWater) {
								g_pTheBM->BlitSurface(pDeepBottom, &src, pDestSurface,
										&dest, bShallowWater ? PIT_SHALLOW_OPACITY : PIT_DEEP_OPACITY);
								if (bShallowWater)
								{
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitSurface(pShallowBottom, &src, pDestSurface,
											&dest, PIT_BOTTOM_OPACITY);
								}
							} else {
								g_pTheBM->BlitWithTileMask(wWaterMask, src,
										pDeepBottom, dest, pDestSurface,
										bShallowWater ? PIT_SHALLOW_OPACITY : PIT_DEEP_OPACITY);
								if (bShallowWater)
								{
									src.x = (wX * CX_TILE) % pShallowBottom->w;
									src.y = (wY * CY_TILE) % pShallowBottom->h;
									g_pTheBM->BlitWithTileMask(wWaterMask, src,
											pShallowBottom,
											dest, pDestSurface, PIT_BOTTOM_OPACITY);
								}
							}
						}

						//5. Draw stepping stones and thin ice on top of water.
						if (bIsSteppingStone(wOTileNo))
						{
							DrawRoomTile(TI_STEP_STONE);
						}
						if (bIsThinIce(wOTileNo))
						{
							const UINT wTileImageNo = CalcTileImageForWater(this->pRoom, wX, wY, T_THINICE);
							DrawTransparentRoomTile(wTileImageNo,THIN_ICE_OPACITY);
						}

						// All lighting is applied when drawing T-Layer
					}
				}
			}

			//Advance all pointers to next square.
			++pTI;
			psL += wLightValuesPerTile;
		}

		//Advance to next row.
		pTI += wRowOffset;
		psL += wLightRowOffset;
	}
	if (pWaterSurface)
		SDL_FreeSurface(pWaterSurface);
}

//*****************************************************************************
void CRoomWidget::DrawTLayerTile(
//Render objects above the o-layer for a room tile onto a surface.
//Applies light and darkness effects (if requested).
//
//Params:
	const UINT wX, const UINT wY,
	const int nX, const int nY,  //used in macros
	SDL_Surface *pDestSurface,
	const UINT wOTileNo,
	const TileImages& ti,
	LIGHTTYPE *psL,
	const float fDark,
	const bool bAddLight,
	const bool bEditor,
	const vector<TweeningTileMask>* pPitPlatformMasks)
{
	ASSERT(this->pRoom);
	const UINT wTTileNo = this->pRoom->GetTSquare(wX, wY);

	//Pits show only dark.  Light only shines on f+t-layer items.
	//Deal with darkening the pit tile now.
	const bool bIsPitTile = bIsPit(wOTileNo) || wOTileNo == T_PLATFORM_P;
	if (bAddLight && bIsPitTile) {
		if (pPitPlatformMasks) {
			//Mask out moving platform tiles when drawing darkness on pits.
			//That is, preclude darkness from being drawn twice on moving platform tiles.
			g_pTheBM->DarkenTileWithMultiTileMask(*pPitPlatformMasks, nX, nY, CX_TILE, CY_TILE, pDestSurface, fDark);
		}
		else if (bIsPit(wOTileNo)) {
			AddDark(fDark);
		}
	}

	bool bTar = bIsTarOrFluff(wTTileNo);
	bool bTIsTransparent = (bTar && bEditor);
	bool bTIsTranslucent = bTar && (this->bRequestTranslucentTar || g_pTheDBM->tarstuffAlpha != 255);

	//2b. Add checkpoints on top of o-layer.
	const bool bIsCheckpoint = (this->bShowCheckpoints || bEditor) &&
			this->pRoom->checkpoints.has(wX,wY);
	if (bIsCheckpoint)
	{
		DrawRoomTile(TI_CHECKPOINT);
		if (bIsPitTile && bAddLight)
			AddLightInterp(pDestSurface, wX, wY, psL, fDark, TI_CHECKPOINT);
	}

	//3. Draw floor (object) layer.
	if (ti.f != TI_TEMPTY)
	{
		DrawRoomTile(ti.f);
		if (bIsPitTile && bAddLight)
			AddLightInterp(pDestSurface, wX, wY, psL, fDark, ti.f);
	}

	//4a. Draw covered item.
	if (ti.tCovered != TI_TEMPTY) {
		DrawRoomTile(ti.tCovered);
		if (bIsPitTile && bAddLight)
			AddLightInterp(pDestSurface, wX, wY, psL, fDark, ti.tCovered);
	}

	//4b. Draw transparent (item) layer.
	bool bIsMoving = false;
	if (ti.t != TI_TEMPTY && !bTar)
	{
		switch (wTTileNo)
		{
			case T_TOKEN:
			{
				//When tokens obscure arrows, make token transparent.
				const UINT fTile = this->pRoom->GetFSquare(wX,wY);
				if (bIsArrow(fTile) || bIsDisabledArrow(fTile))
					DrawTransparentRoomTile(ti.t, 96);
				else
					DrawRoomTile(ti.t);
			}
			break;
			case T_STATION:
			{
				DrawRoomTile(ti.t);
				const UINT tParam = this->pRoom->GetTParam(wX,wY);
				g_pTheBM->LightenRectWithTileMask(pDestSurface, nX, nY,
						CBitmapManager::CX_TILE, CBitmapManager::CY_TILE,
						1.0f+lightMap[0][tParam], 1.0f+lightMap[1][tParam], 1.0f+lightMap[2][tParam],
						ti.t, 0, 0);
			}
			break;
			case T_MIRROR:
			case T_POWDER_KEG:
				//Render moving objects later.
				if (this->dwMovementStepsLeft) {
					bIsMoving = true;
					this->movingTLayerObjectsToRender.insert(wX, wY);
				}
				else
					DrawRoomTile(ti.t);
			break;
			default: DrawRoomTile(ti.t); break;
		}
		if (!bIsMoving) //Moving object's light is applied while it is drawn
			if (bIsPitTile && bAddLight)
				AddLightInterp(pDestSurface, wX, wY, psL, fDark, ti.t);
	}

	if (bEditor) //Area light sprite only shows in room editor.
	{
		UINT lightVal = this->pRoom->tileLights.GetAt(wX,wY);
		if (bIsWallLightValue(lightVal))
			DrawTransparentRoomTile(TI_WALLLIGHT, 180);
	}

	//5. Cast shadows onto environment, except onto pit.
	if (!bIsPitTile)
	{
		if (ti.shadowMasks.size())
		{
			g_pTheBM->BlitTileShadows(&(ti.shadowMasks[0]), ti.shadowMasks.size(), nX, nY, pDestSurface);

			//5b. If this is a high obstacle, no shadow should be cast on any part of it.
			if (wTTileNo == T_OBSTACLE)
			{
				UINT wObSize, wXPos, wYPos;
				GetObstacleStats(this->pRoom, wX, wY, wObSize, wXPos, wYPos);
				if ((wXPos || wYPos) &&
						CastsWallShadow(this->pRoom->GetOSquare(wX - wXPos, wY - wYPos)))
				{
					DrawRoomTile(ti.t);
				}
			}
		}

		//6. Room lighting to light everything on this tile.
		//Pits were handled prior to this
		if (bAddLight)
			AddLightInterp(pDestSurface, wX, wY, psL, fDark);
	}

	//6a. Tarstuff is rendered on top of all light and shadows.
	if (bTar)
	{
		if (wTTileNo == T_FLUFF)
		{
			DrawTransparentRoomTile(ti.t, FLUFF_OPACITY);
		} else if (bTIsTransparent) {
			DrawTransparentRoomTile(ti.t, 128);
		} else if (bTIsTranslucent) {
			static const Uint8 TRANSLUCENT_ALPHA = 166;
			Uint8 tar_alpha = this->bRequestTranslucentTar ? TRANSLUCENT_ALPHA : 255;
			if (g_pTheDBM->tarstuffAlpha < tar_alpha) {
				tar_alpha = g_pTheDBM->tarstuffAlpha;
			}
			DrawTransparentRoomTile(ti.t, tar_alpha);
		} else if (ti.t != TI_TEMPTY) {
			DrawRoomTile(ti.t);
		}
		AddLight(pDestSurface, nX, nY, psL, fDark, ti.t);
	}
}
#undef DrawRoomTile
#undef DrawTransparentRoomTile
#undef AddDark
#undef IsDeepWaterTile
#undef IsShallowTile
#undef IsShallowImage

//*****************************************************************************
void CRoomWidget::Paint(
//Plots current room to display.
//See comments below for an outline of how this is done.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	if (!this->pRoom)
	{
		//Draw black area.
		int nOffsetX, nOffsetY;
		GetScrollOffset(nOffsetX, nOffsetY);
		SURFACECOLOR Color = GetSurfaceColor(GetDestSurface(), 0, 0, 0);
		SDL_Rect rect = MAKE_SDL_RECT(this->x + nOffsetX, this->y + nOffsetY, this->w, this->h);
		DrawFilledRect(rect, Color);
		return;
	}

	const bool bWasApplyingJitter = this->bJitterThisFrame;
	bool bMoveMade;
	UINT wTurn;
	if (this->pCurrentGame)
	{
		SyncRoomPointerToGame(this->pCurrentGame);

		wTurn = this->pCurrentGame->wTurnNo;
		bMoveMade = (wTurn != this->wLastTurn);
	} else {
		bMoveMade = false;
	}
	CDbRoom& room = *(this->pRoom);

	//1a. Prepare vars for drawing this frame.
	if (bMoveMade)
	{
		//Start new movement animation.
		this->wLastTurn = wTurn;
		this->dwCurrentDuration = this->dwTimeSinceLastFrame; //start into move
		ResetUserLightsource();

		HighlightSelectedTile(); //refresh user highlight according to current room state
	}
	if (this->pCurrentGame) {
		SetFrameVars(bMoveMade);
	}

	//1b. Determine animation progress this frame.
	const Uint32 dwNow = SDL_GetTicks();
	if (!this->dwLastFrame)
		dwLastFrame = dwNow;
	this->dwTimeSinceLastFrame = dwNow - dwLastFrame;
	this->dwCurrentDuration += this->dwTimeSinceLastFrame;
	dwLastFrame = dwNow;

	const CSwordsman* player = this->pCurrentGame ? &(this->pCurrentGame->swordsman) : NULL;
	const bool bIsPlacingDouble = this->pCurrentGame ? this->pCurrentGame->swordsman.wPlacingDoubleType != 0 : false;
	this->dwMovementStepsLeft = this->dwCurrentDuration >= this->dwMoveDuration || bIsPlacingDouble ? 0 :
		(CX_TILE - (this->dwCurrentDuration * CX_TILE / this->dwMoveDuration));
	const bool bMoveAnimationInProgress = this->dwMovementStepsLeft || bMoveMade || this->bAnimationInProgress;
	this->bAnimationInProgress = this->dwMovementStepsLeft != 0; //if true, move animation must be drawn next frame to complete it

	//1c. Prepare room image now, if requested.
	SDL_Surface *pDestSurface = GetDestSurface();
	if (!this->pCurrentGame)
	{
		//Render view of the room out of play.
		RenderRoom(this->wShowCol, this->wShowRow,
				CDrodBitmapManager::DISPLAY_COLS, CDrodBitmapManager::DISPLAY_ROWS, false);

		//Blit entire room.
		SDL_Rect src = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_Rect dest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_BlitSurface(this->pRoomSnapshotSurface, &src, pDestSurface, &dest);

		RenderRoomLayers(pDestSurface);

		if (bUpdateRect)
			UpdateRect();
		this->bAllDirty = false;
		return; //once the room has been rendered out of play, there's nothing else to render
	}

	const bool bPlayerIsAlive = !this->pCurrentGame->IsPlayerDying();
	if (!bPlayerIsAlive) // To ensure everything draws properly during death animation everything must be made dirty
		this->bAllDirty = true;

	//Real-time shadow casting animation.
	if (bMoveAnimationInProgress && ShowShadowCastingAnimation())
		AddPlayerLight(true);

	// O-Layers need to be updated and dirtied before O-Layer is drawn to ensure lighting is correctly applied for
	// pieces of the effects that enter a new tile
	this->pOLayerEffects->UpdateEffects();
	this->pTLayerEffects->UpdateEffects();
	this->pMLayerEffects->UpdateEffects();
	this->pLastLayerEffects->UpdateEffects();
	this->pOLayerEffects->DirtyTiles();

	if (this->bRenderRoom || this->bRenderPlayerLight)
	{
		RenderRoomInPlay(this->wShowCol, this->wShowRow);
		this->bRenderRoom = false;
	}

	//1d. Animate monster frames.
	if (!bIsPlacingDouble && bPlayerIsAlive)
		AnimateMonsters();

	//2. Erase all room sprites that were drawn last frame.
	ASSERT(this->pRoomSnapshotSurface);
	ASSERT(this->pTileImages);
	TileImages *pTI = this->pTileImages;
	TileImages *const pTIStop = pTI +
			CDrodBitmapManager::DISPLAY_ROWS * CDrodBitmapManager::DISPLAY_COLS;
	if (this->bAllDirty)
	{
		//Re-blit entire room.
		SDL_Rect src = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_Rect dest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_BlitSurface(this->pRoomSnapshotSurface, &src, pDestSurface, &dest);

		//Undirty all room tiles.
		while (pTI != pTIStop)
		{
			pTI->dirty = pTI->damaged = pTI->monster = 0;
			++pTI;
		}
	} else {
		//Blit only dirtied room tiles from last turn to save time.
		//Undirty them and mark as "damaged" to indicate their screen regions
		//must be updated.
		while (pTI != pTIStop)
			(pTI++)->damaged = 0;
		BlitDirtyRoomTiles(bMoveMade);
	}
	pTI = this->pTileImages;

	//3. Draw room sprites.
	const bool monstersAreAnimated = bMoveAnimationInProgress || bWasApplyingJitter;
	{
		//3a. Draw effects that go on top of room image, under monsters/swordsman.
		RenderFogInPit(pDestSurface);

		DrawPlatformsAndTLayer(pDestSurface, false, bMoveAnimationInProgress);

		this->pTLayerEffects->DrawEffects();
		this->pTLayerEffects->DirtyTiles();

		//3b. Repaint monsters.
		if (this->bAllDirty || monstersAreAnimated)
		{
			//Draw monsters (that aren't killing swordsman).
			DrawMonsters(room.pFirstMonster, pDestSurface, false,
				monstersAreAnimated);
		} else {
			RedrawMonsters(pDestSurface);
		}

		if (bPlayerIsAlive)
			PopulateBuildMarkerEffects(room);

		//4. Draw player.
		if (this->bShowingPlayer && !bIsPlacingDouble && bPlayerIsAlive) // Dying player is rendered later
			DrawPlayer(this->pCurrentGame->swordsman, pDestSurface);

		//X. Debug draws, comment out when you want them to be visible
		//DebugDraw_Pathmap(pDestSurface, MovementType::GROUND);
		//DebugDraw_Pathmap(pDestSurface, MovementType::AIR);
		//DebugDraw_Pathmap(pDestSurface, MovementType::WALL);
		//DebugDraw_Pathmap(pDestSurface, MovementType::WATER);
		//DebugDraw_Pathmap(pDestSurface, MovementType::GROUND_AND_SHALLOW_WATER);
		//DebugDraw_Pathmap(pDestSurface, MovementType::GROUND_FORCE);
		//DebugDraw_Pathmap(pDestSurface, MovementType::GROUND_AND_SHALLOW_WATER_FORCE);
		//DebugDraw_Pathmap(pDestSurface, MovementType::AIR_FORCE);
		//DebugDraw_Pathmap(pDestSurface, MovementType::WATER_FORCE);
		//DebugDraw_MarkedTiles(pDestSurface);

		//5. Draw effects that go on top of monsters/swordsman.
		this->pMLayerEffects->DrawEffects();
		this->pMLayerEffects->DirtyTiles();

		if (player && bIsPlacingDouble && bPlayerIsAlive)
		{
			if (player->wPlacingDoubleType == M_DECOY)
			{
				CCoordIndex coords(this->pRoom->wRoomCols, this->pRoom->wRoomRows);
				for (list<CPlayerDouble*>::const_iterator decoy = this->pRoom->Decoys.begin();
					decoy != this->pRoom->Decoys.end(); ++decoy)
					DrawInvisibilityRange((*decoy)->wX, (*decoy)->wY, pDestSurface, &coords);
			}

			BAndWRect(pDestSurface, this->wShowCol, this->wShowRow);

			ASSERT(this->bShowingPlayer);
			DrawPlayer(*player, pDestSurface);

			DrawOverheadLayer(pDestSurface);
			DrawGhostOverheadCharacters(pDestSurface, false);

			DrawDoubleCursor(this->pCurrentGame->swordsman.wDoubleCursorX,
				this->pCurrentGame->swordsman.wDoubleCursorY, pDestSurface);
			if (this->pCurrentGame->swordsman.wPlacingDoubleType == M_DECOY)
			{
				DrawInvisibilityRange(this->pCurrentGame->swordsman.wDoubleCursorX,
					this->pCurrentGame->swordsman.wDoubleCursorY, pDestSurface);
			}
		} else {
			//6. Draw overhead layer.
			DrawOverheadLayer(pDestSurface);

			//7. Draw ghost overhead images.
			if (this->bAllDirty || monstersAreAnimated) {
				DrawGhostOverheadCharacters(pDestSurface, monstersAreAnimated);
			} else {
				DrawDamagedMonsters(pDestSurface, CDM_GhostOverhead);
			}
		}
	}

	ReduceJitter();

	//8. Environmental effects
	RenderEnvironment(pDestSurface);

	//9. Display filter.
	ApplyDisplayFilterToRoom(getDisplayFilter(), pDestSurface);

	//10. Draw effects that go on top of everything else drawn in the room.
	this->pLastLayerEffects->DrawEffects();
	this->pLastLayerEffects->DirtyTiles();

	RemoveEffectsQueuedForRemoval();

	// When player is dying draw a fade effect on top of everything, and simultaneously fade in
	// player and killing monsters on top of the fade
	if (!bPlayerIsAlive) {
		if (this->fDeathFadeOpacity > 0)
			g_pTheBM->DarkenRect(this->x, this->y, this->w, this->h, 1.0f - this->fDeathFadeOpacity, pDestSurface);

		if (this->bShowingPlayer)
			DrawPlayer(this->pCurrentGame->swordsman, pDestSurface);

		if (!bPlayerIsAlive)
			DrawMonsterKillingPlayer(pDestSurface);
	}


	//Last turn/movement should be drawn completely now.
	this->bFinishLastMovementNow = false;
	this->bRequestEvilEyeGaze = false;

	//If any widgets are attached to this one, draw them now.
	PaintChildren();

	this->pOLayerEffects->DirtyTiles();

	//7. Show changes on screen.
	if (bUpdateRect)
		UpdateRoomRects();

	//Everything has been (re)painted by now.
	this->bAllDirty = false;
}

//*****************************************************************************
void CRoomWidget::RemoveEffectsQueuedForRemoval()
{
	for (multimap<EffectType, int>::const_iterator it=this->queued_layer_effect_type_removal.begin();
			it!=this->queued_layer_effect_type_removal.end(); ++it)
	{
		const EffectType eEffectType = it->first;
		const int layer = it->second;
		RemoveLayerEffects(eEffectType, layer);
	}

	this->queued_layer_effect_type_removal.clear();
}

void CRoomWidget::RemoveGroupEffects(int clearGroup)
{
	ASSERT(this->pOLayerEffects);
	this->pOLayerEffects->RemoveOverlayEffectsInGroup(clearGroup);
	ASSERT(this->pTLayerEffects);
	this->pTLayerEffects->RemoveOverlayEffectsInGroup(clearGroup);
	ASSERT(this->pMLayerEffects);
	this->pMLayerEffects->RemoveOverlayEffectsInGroup(clearGroup);
	ASSERT(this->pLastLayerEffects);
	this->pLastLayerEffects->RemoveOverlayEffectsInGroup(clearGroup);
}

//*****************************************************************************
void CRoomWidget::RemoveLayerEffects(const EffectType eEffectType, int layer)
{
	switch (layer) {
		case 0: RemoveOLayerEffectsOfType(eEffectType); break;
		case 1: RemoveTLayerEffectsOfType(eEffectType); break;
		case 2: RemoveMLayerEffectsOfType(eEffectType); break;
		case 3: RemoveLastLayerEffectsOfType(eEffectType); break;
		default:
			if (layer == ImageOverlayCommand::ALL_LAYERS) {
				RemoveOLayerEffectsOfType(eEffectType);
				RemoveTLayerEffectsOfType(eEffectType);
				RemoveMLayerEffectsOfType(eEffectType);
				RemoveLastLayerEffectsOfType(eEffectType);
			}
		break;
	}
}

//*****************************************************************************
void CRoomWidget::DrawGhostOverheadCharacters(SDL_Surface *pDestSurface, const bool bMoveInProgress)
{
	for (const CMonster* pMonster = this->pRoom->pFirstMonster; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			if (pCharacter->DisplayGhostImage() && pCharacter->IsGhostImageOverhead())
				DrawCharacter(pCharacter, false, pDestSurface, bMoveInProgress);
		}
	}
}

//*****************************************************************************
void CRoomWidget::PaintClipped(int /*nX*/, int /*nY*/, UINT /*wW*/, UINT /*wH*/,
		const bool /*bUpdateRect*/)
{
	//Paint() uses direct access to pixels, so it can't be clipped with
	//SDL_SetClipRect().  Either you need to write PaintClipped() or change
	//the situation which causes this widget to be clipped.
	ASSERT(!"Can't paint clipped.");
}

//*****************************************************************************
void CRoomWidget::PopulateBuildMarkerEffects(const CDbRoom& room)
{
	this->pMLayerEffects->RemoveEffectsOfType(EPENDINGBUILD);

	if (this->puzzleModeOptions.GetHideBuildMarkers())
		return;

	if (!room.building.empty())
	{
		bool bFirst = true;
		for (UINT wY=this->wShowRow; wY<this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS; ++wY)
			for (UINT wX=this->wShowCol; wX<this->wShowCol + CDrodBitmapManager::DISPLAY_COLS; ++wX)
			{
				UINT wTileNo = CALC_NEEDED, wTile = room.building.get(wX,wY);
				bool bRemove = false;
				if (wTile)
				{
					--wTile; //convert from 1-based

					//Display empty item as the o-tile below it.
					if (wTile == T_EMPTY || wTile == T_EMPTY_F || wTile == T_REMOVE_FLOOR_ITEM) {
						wTile = room.GetOSquare(wX,wY);
						bRemove = true;
					}

					switch (wTile)
					{
						case T_WALL_IMAGE:
						case T_WALL2:
						case T_WALL: wTileNo = TI_WALL; break;
						case T_WALL_B: wTileNo = TI_WALL_B; break;
						case T_FUSE: wTileNo = TI_FUSE; break;
						case T_WATER: wTileNo = TI_WATER_TOP; break;
						case T_SHALLOW_WATER: wTileNo = TI_SHALLOW_TOP; break;
						case T_PIT_IMAGE:
						case T_PIT: wTileNo = TI_PIT_M; break;
						case T_GOO: wTileNo = TI_GOO_NSWE; break;
						case T_FLOOR_IMAGE: wTileNo = TI_FLOOR; break;
						case T_DOOR_Y: wTileNo = TI_DOOR_Y; break;
						case T_DOOR_YO: if (!bRemove) wTileNo = TI_DOOR_YO; break;
						case T_DOOR_M: wTileNo = TI_DOOR_M; break;
						case T_DOOR_GO: if (!bRemove) wTileNo = TI_DOOR_GO; break;
						case T_DOOR_B: wTileNo = TI_DOOR_B; break;
						case T_DOOR_BO: if (!bRemove) wTileNo = TI_DOOR_BO; break;
						case T_DOOR_C: wTileNo = TI_DOOR_C; break;
						case T_DOOR_CO: if (!bRemove) wTileNo = TI_DOOR_CO; break;
						case T_DOOR_R: wTileNo = TI_DOOR_R; break;
						case T_DOOR_RO: if (!bRemove) wTileNo = TI_DOOR_RO; break;
						case T_EMPTY: wTileNo = TI_TEMPTY; ASSERT(!"Not valid tile"); break;
						case T_TAR: wTileNo = TI_TAR_NSEW; break;
						case T_MUD: wTileNo = TI_MUD_NSEW; break;
						case T_GEL: wTileNo = TI_GEL_NSEW; break;
						case T_FLUFF: wTileNo = TI_FLUFF_NSEW; break;
						case T_ORB_CRACKED: wTileNo = TI_ORB_CRACKING; break;
						case T_ORB_BROKEN: wTileNo = TI_ORB_CRACKED; break;
						case T_ORB_NORMAL: wTileNo = TI_ORB_D; break;
						case T_ORB: wTileNo = TI_ORB_D; break;
						case T_FLOOR_SPIKES: wTileNo = TI_FLOOR_SPIKES_DOWN; break;
						case T_WALL_H: wTileNo = TI_WALL_H; break;
						case T_THINICE:
						case T_THINICE_SH: wTileNo = TI_THINICE; break;
						case T_BRIAR_DEAD: wTileNo = TI_BRIAR; break;
						case T_BRIAR_LIVE: wTileNo = TI_BRIAR2; break;
						case T_BRIAR_SOURCE: wTileNo = TI_BRIARROOT; break;
						default: wTileNo = GetTileImageForTileNo(wTile); break;
					}
					if (bIsFakeTokenType(wTile))
						wTileNo = CalcTileImageForToken(ConvertFakeTokenType(wTile));
					if (wTileNo == CALC_NEEDED)
						wTileNo = CalcTileImageFor(&room, wTile, wX, wY);
					if (wTileNo != CALC_NEEDED)
						this->pMLayerEffects->AddEffect(new CPendingBuildEffect(
								this, wTileNo, wX, wY, bFirst));
					bFirst = false;
				}
			}
	}
}

//*****************************************************************************
void CRoomWidget::RedrawMonsters(SDL_Surface* pDestSurface)
{
	//Paint monsters whose tiles have been repainted.
	DrawDamagedMonsters(pDestSurface, CDM_GhostFloor);
	DrawDamagedMonsters(pDestSurface, CDM_Normal);

	//Redraw monster and NPC swords to ensure no other monster is being drawn on top of them.
	if (!this->dwMovementStepsLeft)
	{
		DrawDamagedMonsterSwords(pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::RenderEnvironment(SDL_Surface *pDestSurface)	//[default=NULL]
//(Re)draw environmental effects on tiles being redrawn.
{
	static float fBrilliance = 1.0;	//lightning

	if (!IsWeatherRendered() || this->puzzleModeOptions.GetHideWeather())
		return;

	bool bHasted = false, bIsPlacingDouble = false;
	if (this->pCurrentGame)
	{
		const CSwordsman& player = this->pCurrentGame->swordsman;
		bHasted = player.bIsHasted;
		bIsPlacingDouble = player.wPlacingDoubleType != 0;
	}

	// Prevent creating new snowflakes/raindrobs when they couldn't animate and disappear, to prevent
	// game crashing/lagging, looking weird
	if (!this->pMLayerEffects->GetEffectsFrozen()) {
		//Add a new snowflake to the room every ~X frames.
		if (this->wSnow && RAND(SNOW_INCREMENTS - 1) < this->wSnow &&
			this->w && this->y) //hack: snowflakes draw on room edges during transition -- this should stop it
			AddMLayerEffect(new CSnowflakeEffect(this));

		//Add a new raindrop to the room every ~X frames.
		if (this->rain && RAND(RAIN_INCREMENTS - 1) < this->rain && !bIsPlacingDouble &&
			this->w && this->y) //hack: rain draws on room edges during transition -- this should stop it
			AddMLayerEffect(new CRaindropEffect(this, bHasted));
	}

	if (!(this->dwLightning || this->bFog || this->bClouds || this->bSunlight))
		return;	//Nothing else to do.

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Optimized room-wide blits can be handled here.
	if (this->bAllDirty)
	{
		if (this->dwLightning)
		{
			//Make lightning flicker.  Should only change when entire screen is updated.
			fBrilliance = 1.0f + fMaxLightIntensity[this->wDark] * fRAND(1.0f);
		}
	}

	//Tile-by-tile handling is done here.
	TileImages *pTI = this->pTileImages;
	CCoordSet dirtyTiles;
	const UINT wXEnd = this->wShowCol + CDrodBitmapManager::DISPLAY_COLS;
	const UINT wYEnd = this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS;

	bool bShowFogHere = false;
	for (UINT wY = this->wShowRow; wY < wYEnd; ++wY)
	{
		for (UINT wX = this->wShowCol; wX < wXEnd; ++wX)
		{
			if (this->bAllDirty || pTI->damaged)
			{
				dirtyTiles.insert(wX,wY);

				//Draw effects bottom-up.
				const UINT wOSquare = this->pRoom->GetOSquare(wX,wY);

				if (!(bIsPit(wOSquare) || wOSquare == T_PLATFORM_P))
				{
					if (this->bSunlight)
					{
						SDL_Rect src = MAKE_SDL_RECT(wX * CX_TILE, wY * CY_TILE, CX_TILE, CY_TILE);
						SDL_Rect dest = MAKE_SDL_RECT(this->x + wX * CX_TILE, this->y + wY * CY_TILE, CX_TILE, CY_TILE);
						const vector<UINT> &shadows = pTI->shadowMasks;
						//Determine what type of cloud behavior would be most consistent to model.
						//The open sky model is given priority, otherwise free-moving clouds are used.
						if (this->bClouds && !this->bSkyVisible)
						{
							const int wXOffset = (int)this->fCloudX % this->images[SUNSHINE_SURFACE]->w;
							const int wYOffset = (int)this->fCloudY % this->images[SUNSHINE_SURFACE]->h;
							src.x += wXOffset;
							src.y += wYOffset;
						} else {
							//Cloud shadows move faster over the ground than in the reflection of the sky.
							static const UINT wShadowSpeedMultiplier = 3;
							const int wXOffset = (this->dwSkyX * wShadowSpeedMultiplier)
								% this->images[SUNSHINE_SURFACE]->w;
							src.x += wXOffset;
						}
						const UINT* pwShadow = NULL;
						UINT numShadows = 0;
						if (g_pTheBM->bAlpha && !shadows.empty()) {
							pwShadow = &(*shadows.begin());
							numShadows = shadows.size();
						}
						g_pTheBM->ShadeWithWrappingSurfaceMask(this->images[SUNSHINE_SURFACE], src,
								pDestSurface, dest, pwShadow, numShadows);
					}

					if (this->dwLightning)
						g_pTheBM->LightenTile(pDestSurface,
							this->x + wX*CX_TILE, this->y + wY*CY_TILE, fBrilliance);

					//Handles blitting full fog above floor/walls.
					{
						switch (this->cFogLayer)
						{
							default: break;
							case 2: //over floor
								bShowFogHere = !(bIsWall(wOSquare) || bIsCrumblyWall(wOSquare) || bIsDoor(wOSquare));
							break;
							case 3: //over walls (i.e. everything)
								bShowFogHere = true;
							break;
						}
						if (bShowFogHere)
						{
							const int wXOffset = (int)this->fFogX % this->images[FOG_SURFACE]->w;
							const int wYOffset = (int)this->fFogY % this->images[FOG_SURFACE]->h;

							//Fog with uniform alpha.

							//When darkness is added to a tile with fog, show fog that much more faintly.
							const float fDark = GetOverheadDarknessAt(wX, wY);
							const Uint8 opacity = Uint8(MIN_FOG_OPACITY * fDark);
							if (opacity) {
								SDL_Rect src = MAKE_SDL_RECT((int)(wXOffset + wX * CX_TILE), (int)(wYOffset + wY * CY_TILE), CX_TILE, CY_TILE);
								SDL_Rect dest = MAKE_SDL_RECT(this->x + wX * CX_TILE, this->y + wY * CY_TILE, CX_TILE, CY_TILE);
								EnableSurfaceBlending(this->images[FOG_SURFACE], opacity);
								g_pTheBM->BlitWrappingSurface(this->images[FOG_SURFACE], src, pDestSurface, dest);
							}
						}
					}
				}
			}
			++pTI;
		}
	}

	for (vector<CPlatform*>::const_iterator platformIter = this->pRoom->platforms.begin();
			platformIter != this->pRoom->platforms.end(); ++platformIter)
	{
		const CPlatform& platform = *(*platformIter);

		//ignore water platforms
		if (platform.GetTypes().has(T_WATER)) continue;

		const bool bPlatformMoved = platform.xDelta || platform.yDelta;

		//Calculate animation offset (in pixels).
		UINT wXOffset = 0, wYOffset = 0;
		if (bPlatformMoved && this->dwMovementStepsLeft)
		{
			wXOffset = -platform.xDelta * this->dwMovementStepsLeft;
			wYOffset = -platform.yDelta * this->dwMovementStepsLeft;
		}

		CCoordSet tiles;
		platform.GetTiles(tiles);

		for (CCoordSet::const_iterator tileIter = tiles.begin();
				tileIter != tiles.end(); ++tileIter)
		{
			UINT wX = tileIter->wX, wY = tileIter->wY;

			if (this->bAllDirty || dirtyTiles.has(wX,wY))
			{
				UINT wXDest = wX * CX_TILE + wXOffset;
				UINT wYDest = wY * CY_TILE + wYOffset;
				if (this->bSunlight)
				{
					SDL_Rect src = MAKE_SDL_RECT(wXDest, wYDest, CX_TILE, CY_TILE);
					SDL_Rect dest = MAKE_SDL_RECT(this->x + wXDest, this->y + wYDest, CX_TILE, CY_TILE);

					const vector<UINT> &shadows = this->pTileImages[this->pRoom->ARRAYINDEX(wX,wY)].shadowMasks;
					const UINT* pwShadow = NULL;
					if (!bPlatformMoved && (shadows.size() > 0))
						pwShadow = &(shadows[0]);

					//Determine what type of cloud behavior would be most consistent to model.
					//The open sky model is given priority, otherwise free-moving clouds are used.
					if (this->bClouds && !this->bSkyVisible)
					{
						const int wCXOffset = (int)this->fCloudX % this->images[SUNSHINE_SURFACE]->w;
						const int wCYOffset = (int)this->fCloudY % this->images[SUNSHINE_SURFACE]->h;
						src.x += wCXOffset;
						src.y += wCYOffset;
					} else {
						//Cloud shadows move faster over the ground than in the reflection of the sky.
						static const UINT wShadowSpeedMultiplier = 3;
						const int wCXOffset = (this->dwSkyX * wShadowSpeedMultiplier)
							% this->images[SUNSHINE_SURFACE]->w;
						src.x += wCXOffset;
					}
					g_pTheBM->ShadeWithWrappingSurfaceMask(this->images[SUNSHINE_SURFACE], src,
						pDestSurface, dest, pwShadow, bPlatformMoved ? 0 : shadows.size());
				}

				if (this->dwLightning)
					g_pTheBM->LightenTile(pDestSurface,
						this->x + wXDest, this->y + wYDest, fBrilliance);

				//Handles blitting full fog above floor/walls.
				if (this->cFogLayer >= 2)
				{
					const int wXOffset = (int)this->fFogX % this->images[FOG_SURFACE]->w;
					const int wYOffset = (int)this->fFogY % this->images[FOG_SURFACE]->h;

					//Fog with uniform alpha.
					//When darkness is added to a tile with fog, show fog that much more faintly.
					const float fDark = GetOverheadDarknessAt(wX, wY);
					const Uint8 opacity = Uint8(MIN_FOG_OPACITY * fDark);
					if (opacity) {
						SDL_Rect src = MAKE_SDL_RECT((int)(wXOffset + wXDest), (int)(wYOffset + wYDest), CX_TILE, CY_TILE);
						SDL_Rect dest = MAKE_SDL_RECT(this->x + wXDest, this->y + wYDest, CX_TILE, CY_TILE);
						EnableSurfaceBlending(this->images[FOG_SURFACE], opacity);
						g_pTheBM->BlitWrappingSurface(this->images[FOG_SURFACE], src, pDestSurface, dest);
					}
				}
			}
		}
	}

	if (this->bClouds)
	{
		for (CCoordSet::const_iterator tile=dirtyTiles.begin(); tile!=dirtyTiles.end(); ++tile)
		{
			UINT wX = tile->wX;
			UINT wY = tile->wY;

			SDL_Rect src = MAKE_SDL_RECT((int)(wX * CX_TILE), (int)(wY * CY_TILE), CX_TILE, CY_TILE);
			SDL_Rect dest = MAKE_SDL_RECT(this->x + wX * CX_TILE, this->y + wY * CY_TILE, CX_TILE, CY_TILE);
			//If sky is visible, move clouds the same way.
			//Otherwise use the free-moving cloud movement.
			if (this->bSkyVisible)
			{
				static const UINT wCloudSpeedMultiplier = 2;
				const int wXOffset = (this->dwSkyX * wCloudSpeedMultiplier)
					% this->images[CLOUD_SURFACE]->w;
				src.x += wXOffset;
			} else {
				const int wXOffset = (int)this->fCloudX % this->images[CLOUD_SURFACE]->w;
				const int wYOffset = (int)this->fCloudY % this->images[CLOUD_SURFACE]->h;
				src.x += wXOffset;
				src.y += wYOffset;
			}
			g_pTheBM->BlitWrappingSurface(this->images[CLOUD_SURFACE], src, pDestSurface, dest);
		}
	}
}

//*****************************************************************************
void CRoomWidget::RenderFogInPit(SDL_Surface *pDestSurface) //[default=NULL]
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	if (!this->bFog)
		return;

	//Tile-by-tile handling is done here.
	TileImages *pTI = this->pTileImages;
	const UINT wXEnd = this->wShowCol + CDrodBitmapManager::DISPLAY_COLS;
	const UINT wYEnd = this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS;

	const UINT pit_image_height = static_cast<UINT>(g_pTheDBM->pTextures[PITSIDE_MOSAIC]->h);
	const UINT wMaxPitHeight = (pit_image_height / CY_TILE) +
			((pit_image_height % CY_TILE) > 0 ? 1 : 0);
	const float fFogMultiplier = float(MAX_FOG_OPACITY) / float(wMaxPitHeight);

	for (UINT wY = this->wShowRow; wY < wYEnd; ++wY)
	{
		for (UINT wX = this->wShowCol; wX < wXEnd; ++wX)
		{
			if (this->bAllDirty || pTI->damaged)
			{
				const UINT wOSquare = this->pRoom->GetOSquare(wX,wY);

				if (bIsPit(wOSquare) || wOSquare == T_PLATFORM_P) //render fog under platform
				{
					//Show fog here.
					const int wXOffset = (int)this->fFogX % this->images[FOG_SURFACE]->w;
					const int wYOffset = (int)this->fFogY % this->images[FOG_SURFACE]->h;
					UINT wPitHeight = pTI->edges.wPitY;

					int srcX = (int)(wXOffset + wX * CX_TILE);
					int srcY = (int)(wYOffset + wY * CY_TILE);
					int destX = this->x + wX * CX_TILE;
					int destY = this->y + wY * CY_TILE;

					//Don't draw over tile edging.
					UINT wPixelWidth = pTI->edges.east ? CX_TILE-1 : CX_TILE;
					if (pTI->edges.west) {
						++srcX;
						++destX;
						--wPixelWidth;
					}
					UINT wPixelHeight = pTI->edges.south ? CY_TILE-1 : CY_TILE;
					if (pTI->edges.north) {
						++srcY;
						++destY;
						--wPixelHeight;
					}

					//When darkness is added to a tile with fog, show fog that much more faintly.
					const float fDark = GetOverheadDarknessAt(wX, wY);
					if (wPitHeight >= wMaxPitHeight)
					{
						//Fog with uniform alpha.
						const Uint8 opacity = Uint8(MAX_FOG_OPACITY * fDark);
						if (opacity) {
							SDL_Rect src = MAKE_SDL_RECT(srcX, srcY, wPixelWidth, wPixelHeight);
							SDL_Rect dest = MAKE_SDL_RECT(destX, destY, wPixelWidth, wPixelHeight);
							EnableSurfaceBlending(this->images[FOG_SURFACE], opacity);
							g_pTheBM->BlitWrappingSurface(this->images[FOG_SURFACE], src, pDestSurface, dest);
						}
					} else {
						//Fog with graded alpha.
						//Draw one pixel row at a time.
						SDL_Rect src = MAKE_SDL_RECT(srcX, srcY, wPixelWidth, 1);
						SDL_Rect dest = MAKE_SDL_RECT(destX, destY, wPixelWidth, 1);
						const Uint8 minOpacity = Uint8(MIN_FOG_OPACITY * fDark);
						for (UINT row=0; row<wPixelHeight; ++row)
						{
							const float row_height = wPitHeight + row/float(CY_TILE);
							Uint8 nOpacity = Uint8(row_height * fFogMultiplier * fDark);
							if ((this->cFogLayer > 1) && (nOpacity < minOpacity))
								nOpacity = minOpacity;
							if (nOpacity) {
								EnableSurfaceBlending(this->images[FOG_SURFACE], nOpacity);
								g_pTheBM->BlitWrappingSurface(this->images[FOG_SURFACE], src, pDestSurface, dest);
							}
							++src.y;
							++dest.y;
						}
					}
				}
			}

			++pTI;
		}
	}
}

//*****************************************************************************
void CRoomWidget::ResetJitter()
//Reset jitter info for the room.
{
	this->jitterInfo.Clear();
}

//*****************************************************************************
void CRoomWidget::UpdateRoomRects()
//Show room changes on screen.
{
	if (this->bAllDirty)
	{
		//Whole room was redrawn, thus it should be updated on screen.
		UpdateRect();
		return;
	}

	//Don't damage the entire room region.  Damaging only those tiles that
	//we know were actually drawn to will take much less time.
	TileImages *pTI = this->pTileImages;

	//Count how many runs of rectangles are being blitted.
	UINT wDirtyCount = 0;
	bool bInDirtyRect = false;
	const UINT rows = this->pRoom->wRoomRows;
	const UINT cols = this->pRoom->wRoomCols;
	UINT wX, wY;
	for (wY = 0; wY < rows; ++wY)
	{
		for (wX = 0; wX < cols; ++wX)
		{
			if (pTI->damaged || pTI->dirty)
			{
				if (!bInDirtyRect)
				{
					++wDirtyCount;
					bInDirtyRect = true;
				}
			} else {
				bInDirtyRect = false;
			}
			++pTI;
		}
		bInDirtyRect = false; //row run stops
	}
	if (wDirtyCount > 50)
	{
		//We're gonna be re-blitting a lot of room pieces.  Calling a single
		//update for the whole room will probably be less expensive.
		UpdateRect();
		return;
	}

	//Update tiles where something needs to redrawn on screen.
	//This includes tiles that had old sprites erased this frame (damaged),
	//as well as tiles where something new was drawn this frame (dirty).
	pTI = this->pTileImages;
	UINT wStartIndex = 0;
	for (wY = 0; wY < rows; ++wY)
	{
		for (wX = 0; wX < cols; ++wX)
		{
			if (pTI->damaged || pTI->dirty)
			{
				if (!bInDirtyRect)
				{
					wStartIndex = pTI - this->pTileImages;
					bInDirtyRect = true;
				}
			} else if (bInDirtyRect) {
				//Damage the run of tiles up until this one.
				const UINT wAfterEndIndex = pTI - this->pTileImages;
				UpdateRect(this->x + (wStartIndex%CDrodBitmapManager::DISPLAY_COLS)*CX_TILE,
						this->y + (wStartIndex/CDrodBitmapManager::DISPLAY_COLS)*CY_TILE,
						CX_TILE * (wAfterEndIndex - wStartIndex), CY_TILE);
				bInDirtyRect = false;
			}
			++pTI;
		}
		if (bInDirtyRect)
		{
			const UINT wAfterEndIndex = pTI - this->pTileImages;
			UpdateRect(this->x + (wStartIndex%CDrodBitmapManager::DISPLAY_COLS)*CX_TILE,
					this->y + (wStartIndex/CDrodBitmapManager::DISPLAY_COLS)*CY_TILE,
					CX_TILE * (wAfterEndIndex - wStartIndex), CY_TILE);
			bInDirtyRect = false;
		}
	}
}

//*****************************************************************************
void CRoomWidget::RemoveLastLayerEffectsOfType(
//Removes all last-layer effects of given type in the room.
//
//Params:
	const EffectType eEffectType, //(in) Type of effect to remove.
	const bool bForceClearAll) //if set [default=true], delete all effects,
	                     //including those that request to be retained
{
	ASSERT(this->pLastLayerEffects);
	this->pLastLayerEffects->RemoveEffectsOfType(eEffectType, bForceClearAll);
}

//*****************************************************************************
void CRoomWidget::RemoveMLayerEffectsOfType(
//Removes all m-layer effects of given type in the room.
//
//Params:
	const EffectType eEffectType) //(in) Type of effect to remove.
{
	ASSERT(this->pMLayerEffects);
	this->pMLayerEffects->RemoveEffectsOfType(eEffectType);
}

//*****************************************************************************
void CRoomWidget::RemoveTLayerEffectsOfType(
//Removes all t-layer effects of given type in the room.
//
//Params:
	const EffectType eEffectType) //(in) Type of effect to remove.
{
	ASSERT(this->pTLayerEffects);
	this->pTLayerEffects->RemoveEffectsOfType(eEffectType);
}

void CRoomWidget::RemoveOLayerEffectsOfType(const EffectType eEffectType)
{
	ASSERT(this->pOLayerEffects);
	this->pOLayerEffects->RemoveEffectsOfType(eEffectType);
}

//*****************************************************************************
void CRoomWidget::SetOpacityForMLayerEffectsOfType(
//Set opacity for all m-layer effects of given type in the room.
//
//Params:
	const EffectType eEffectType, //(in) Type of effect to remove.
	float fOpacity)
{
	ASSERT(this->pMLayerEffects);
	this->pMLayerEffects->SetOpacityForEffectsOfType(eEffectType, fOpacity);
}

//*****************************************************************************
void CRoomWidget::SetOpacityForEffectsOfType(
//Set opacity for all m-layer effects of given type in the room.
//
//Params:
	const EffectType eEffectType, //(in) Type of effect to remove.
	float fOpacity)
{
	this->pOLayerEffects->SetOpacityForEffectsOfType(eEffectType, fOpacity);
	this->pTLayerEffects->SetOpacityForEffectsOfType(eEffectType, fOpacity);
	this->pMLayerEffects->SetOpacityForEffectsOfType(eEffectType, fOpacity);
	this->pLastLayerEffects->SetOpacityForEffectsOfType(eEffectType, fOpacity);
}

//*****************************************************************************
void CRoomWidget::GetSquareRect(
//Get rect on screen surface of a specified square.
//
//Params:
	UINT wCol, UINT wRow,   //(in)   Square to get rect for.
	SDL_Rect &SquareRect)   //(out)  Receives rect.
const
{
	ASSERT(IS_COLROW_IN_DISP(wCol, wRow));
	SET_RECT(SquareRect, this->x + (CX_TILE * wCol),
			this->y + (CY_TILE * wRow), CX_TILE,
			CY_TILE);
}

//*****************************************************************************
UINT CRoomWidget::GetOrbMID(const UINT type)
//Returns: messageID corresponding to pressure plate type
{
	switch (type)
	{
		default: return TILE_MID[T_ORB];
		case OT_ONEUSE: return MID_OrbCracked;
		case OT_BROKEN: return MID_OrbBroken;
	}
}

//*****************************************************************************
UINT CRoomWidget::GetPressurePlateMID(const UINT type)
//Returns: messageID corresponding to pressure plate type
{
	switch (type)
	{
		default: return TILE_MID[T_PRESSPLATE];
		case OT_TOGGLE: return MID_PressurePlateToggle;
		case OT_ONEUSE: return MID_PressurePlateOneUse;
		case OT_BROKEN: return MID_PressurePlateBroken;
	}
}

//*****************************************************************************
UINT CRoomWidget::GetTokenMID(const UINT param)
//Returns: messageID corresponding to token type
{
	switch (calcTokenType(param))
	{
		case RotateArrowsCW:
		  return bTokenActive(param) ? MID_TokenRotateCCW : MID_Token;
		case RotateArrowsCCW:
		  return bTokenActive(param) ? MID_Token : MID_TokenRotateCCW;
		case SwitchTarMud: return MID_TokenTarMud;
		case SwitchTarGel: return MID_TokenTarGel;
		case SwitchGelMud: return MID_TokenGelMud;
		case TarTranslucent: return MID_TokenTranslucentTar;
		case PowerTarget: return MID_TokenPowerTarget;
		case WeaponDisarm: return MID_TokenSwordDisarm;
		case PersistentCitizenMovement: return MID_TokenCitizen;
		case ConquerToken: return MID_TokenConquer;
		case SwordToken: return MID_TokenSword;
		case PickaxeToken: return MID_TokenPickaxe;
		case SpearToken: return MID_TokenSpear;
		case StaffToken: return MID_TokenStaff;
		case DaggerToken: return MID_TokenDagger;
		case CaberToken: return MID_TokenCaber;
		case TemporalSplit: return MID_TemporalSplit;
		case TemporalSplitUsed: return MID_TemporalSplitUsed;
		default: ASSERT(!"Unexpected token type."); return 0;
	}
}

//*****************************************************************************
UINT CRoomWidget::SwitchAnimationFrame(
//Switch the monster animation frame at specified tile.
//
//Params:
	const UINT wCol, const UINT wRow)   //(in)   Room square.
{
	ASSERT(wCol < this->pRoom->wRoomCols);
	ASSERT(wRow < this->pRoom->wRoomRows);
	TileImages *pTile = this->pTileImages + this->pRoom->ARRAYINDEX(wCol,wRow);
	if (++pTile->animFrame >= ANIMATION_FRAMES) //increment frame
		pTile->animFrame = 0;
	return UINT(pTile->animFrame);
}

//*****************************************************************************
UINT CRoomWidget::AdvanceAnimationFrame(
//Advance the monster animation frame at specified tile.
//
//Params:
	const UINT wCol, const UINT wRow)   //(in)   Room square.
{
	ASSERT(wCol < this->pRoom->wRoomCols);
	ASSERT(wRow < this->pRoom->wRoomRows);
	TileImages *pTile = this->pTileImages + this->pRoom->ARRAYINDEX(wCol,wRow);
	return UINT(++pTile->animFrame);
}

//*****************************************************************************
void CRoomWidget::AdvanceAnimationFrame(
//Advance the advanced monster animation frame.
//
//Params:
	const CMonster *pMonster)
{
	static const UINT randomPauseDelay = 1000;
	static const UINT randomPauseDuration = 100;

	std::map<const CMonster*, MonsterAnimation>::iterator it = this->monsterAnimations.find(pMonster);
	if (it == this->monsterAnimations.end()) {
		const UINT delay = 10 + RAND(50);
		it = this->monsterAnimations.insert(make_pair(
			pMonster, 
			MonsterAnimation(RAND(ANIMATION_FRAMES), RAND(delay), delay, RAND(randomPauseDelay), RAND(randomPauseDuration))
		)).first;
	}

	MonsterAnimation& mAnimation = it->second;

	if (mAnimation.pauseTimer > 0) {
		if (--mAnimation.pauseTimer == 0)
			mAnimation.waitUntilPause = RAND(randomPauseDelay);
	} else if (mAnimation.waitUntilPause == 0) {
		mAnimation.pauseTimer = RAND(randomPauseDuration);
	} else {
		--mAnimation.waitUntilPause;
		++mAnimation.frameTimerPosition;

		if (mAnimation.frameTimerPosition >= mAnimation.frameDuration) {
			++mAnimation.currentFrame;
			mAnimation.frameTimerPosition = 0;
		}
	}

	TileImages *pTile = this->pTileImages + this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY);
	pTile->animFrame = mAnimation.currentFrame;
}

//
//Protected methods.
//

//*****************************************************************************
CRoomWidget::~CRoomWidget()
//Destructor.  Use Unload() for cleanup.
{
	ASSERT(!this->bIsLoaded);
	delete this->pLastLayerEffects;
	delete this->pMLayerEffects;
	delete this->pOLayerEffects;
	delete this->pTLayerEffects;

	if (this->pRoomSnapshotSurface)
	{
		SDL_FreeSurface(this->pRoomSnapshotSurface);
		this->pRoomSnapshotSurface = NULL;
	}
}

//*****************************************************************************
bool CRoomWidget::Load()
//Load resources for CRoomWidget.
//
//Returns:
//True if successful, false if not.
{
	if (!CWidget::Load())
		return false;

	static Uint32 TransparentColor = SDL_MapRGB(this->images[BOLTS_SURFACE]->format,
			TRANSPARENT_RGB);
	SetColorKey(this->images[BOLTS_SURFACE], SDL_TRUE, TransparentColor);

	if (this->pCurrentGame)
		if (!LoadFromCurrentGame(this->pCurrentGame))
			return false;

	return true;
}

//*****************************************************************************
void CRoomWidget::Unload()
//Unload resources for CRoomWidget.
{
	UnloadCurrentGame();

	CWidget::Unload();
}

//*****************************************************************************
void CRoomWidget::UnloadCurrentGame()
{
	this->pHighlitMonster = NULL;
	this->pCurrentGame = NULL;
	this->pRoom = NULL;
	this->dwLastDrawSquareInfoUpdateCount = 0;
	this->monsterAnimations.clear();

	//Force state reload.
	ClearEffects();
	DeleteArrays();
}

//*****************************************************************************
void CRoomWidget::DeleteArrays()
//Deallocate arrays.
{
	delete[] this->pTileImages;
	this->pTileImages = NULL;

	this->lightMaps.clear();
}

//*****************************************************************************
void CRoomWidget::SetFrameVars(const bool bMoveMade)
//Update information that is used for rendering this frame.
{
	static bool bOpacityUp = false;

	//Stationary view.
	if (!this->bAnimateMoves)
	{
		this->dwMovementStepsLeft = 0;
		this->ghostOpacity = 255;
		this->temporalCloneEffectHeight = -1;
		bOpacityUp = false;
		return;
	}

	this->bJitterThisFrame = false;

	//Do player and monsters need to be repainted?
	if (bMoveMade)
	{
		if (this->dwMovementStepsLeft)
		{
			//The previous move was still being rendered -- skip it and begin new one now.
			this->bFinishLastMovementNow = true;
			//A complete refresh must occur to get everything to update correctly.
			//This makes the animation jerk.
			//To prevent it, call FinishMoveAnimation and Paint on this object
			//to force finishing the animation of the last move before this move is made.
			this->bAllDirty = true;
		}

		SetMoveCountText();

		//Ensure sleeping is stopped.
		StopSleeping();

		BetterVisionQuery();
	} else {
		//Handle graphic animations specific to between moves only.
		const UINT dwTimeToSleep = 21000;   //21s -- longer than EXIT_DELAY in GameScreen.cpp
		if (!this->bAllowSleep)
		{
			//This will keep from falling asleep right after bAllowSleep is set.
			if (this->dwCurrentDuration > this->dwMoveDuration)
				this->dwCurrentDuration = this->dwMoveDuration;
			AllowSleep(false);
		} else if (this->dwCurrentDuration >= dwTimeToSleep)
		{
			//Effect shows player asleep after no action for a while in peaceful rooms.
			if (!this->bPlayerSleeping)
			{
				ASSERT(this->pCurrentGame);
				CSwordsman& player = this->pCurrentGame->swordsman;

				this->bPlayerSleeping = true;
				CMoveCoord origin(player.wX, player.wY, N);
				AddMLayerEffect(new CFloatEffect(this, origin, TI_ZSLEEP, 4, 6));
				if (this->pParent && this->pParent->GetType() == WT_Screen)
				{
					CScreen *pScreen = DYN_CAST(CScreen*, CWidget*, this->pParent);
					ASSERT(pScreen);
					if (pScreen->GetScreenType() == SCR_Game)
					{
						CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*, pScreen);
						ASSERT(pGameScreen);
						pGameScreen->GetFaceWidget()->SetSleeping(true);
					}
				}
			}
			if (!g_pTheSound->IsSoundEffectPlaying(SEID_SNORING))
				g_pTheSound->PlaySoundEffect(SEID_SNORING);
		}
	}

	const Uint32 dwNow = SDL_GetTicks();

	//Slayer wisp animation.
	static const Uint32 dwWispAnimationRate = 100;	//ms
	static Uint32 dwLastWispAnimation = 0;
	if (dwNow - dwLastWispAnimation < dwWispAnimationRate)
		this->bNextWispFrame = false;
	else
	{
		this->bNextWispFrame = true;
		dwLastWispAnimation = dwNow;
	}

	//Update wall monster transparency.
	if (bOpacityUp)
	{
		if (this->ghostOpacity > 235) {
			bOpacityUp = false;
		} else {
			this->ghostOpacity += 3;
		}
	} else {
		if (this->ghostOpacity < 35) {
			bOpacityUp = true;
		} else {
			this->ghostOpacity -= 3;
		}
	}

	//Temporal clone animation.
	static const float fTemporalCloneEffectDuration = 1300.0f; //ms
	this->temporalCloneEffectHeight = int(UINT((dwNow / fTemporalCloneEffectDuration) * CBitmapManager::CY_TILE)) % CBitmapManager::CY_TILE;

	//Environmental effects.
	//These are mostly frame-based, not time-based, in order to look good
	//while keeping framerate high on older systems.
	const bool bEnvironmentalEffects = g_pTheBM->bAlpha && IsWeatherRendered();
	if (bEnvironmentalEffects) {
		SetFrameVarsForWeather();
	}
}

void CRoomWidget::SetFrameVarsForWeather()
{
	static const float fMaxVelocity = 1 / (float)CDrodBitmapManager::DISPLAY_ROWS;
	static const int HALF_ROWS = CDrodBitmapManager::DISPLAY_ROWS / 2;

	ASSERT(this->pCurrentGame);
	CSwordsman& player = this->pCurrentGame->swordsman;
	const bool bIsPlacingDouble = player.wPlacingDoubleType != 0;
	if (bIsPlacingDouble)
		return;

	const bool bPlayerIsAlive = !this->pCurrentGame->IsPlayerDying();

	const Uint32 dwNow = SDL_GetTicks();

	//1. Lightning.  It comes first because it might cause a full refresh,
	//which affects how other effects are rendered.
	if (!this->bLightning) {
		this->dwLightning = 0;
	} else {
		if (!this->dwLightning && bPlayerIsAlive)
		{
			//Randomly cause a lightning flash.
			if (RAND(2500) <= 2) //about once every 20 seconds
			{
				this->dwLightning = dwNow;
				this->bAllDirty = true;
				const Uint32 time_of_thunder = dwNow + Uint32(1000 + fRAND_MID(250.0f)); //about a second later
				this->playThunder.push(time_of_thunder);
			}
		} else {
			//Stop lightning after a moment.
			static const Uint32 dwLightningDuration = 350;
			if (this->dwLightning + dwLightningDuration < dwNow || !bPlayerIsAlive)
			{
				this->dwLightning = 0;
				this->bAllDirty = true;
			} else {
				//Change lightning intensity occasionally while flash continues.
				if (RAND(8) == 0)
					this->bAllDirty = true;
			}
		}
	}

	//2. Sunlight through clouds is modeled based on either the movement of the open sky
	//or clouds being explicitly drawn.  The former is given priority, otherwise
	//the free-moving clouds model is used.
	if ((this->bSkyVisible || (this->bSunlight && !this->bClouds)))
	{
		//Update sky state periodically.
		if (!this->redrawingRowForWeather)
		{
			static const Uint32 MAX_SKY_FRAMES_PER_SECOND = 12;
			static const Uint32 MIN_TIME_BETWEEN_SKY_UPDATES = 1000/MAX_SKY_FRAMES_PER_SECOND; //ms
			if (dwNow >= this->time_of_last_sky_move + MIN_TIME_BETWEEN_SKY_UPDATES) {
				this->time_of_last_sky_move = dwNow;

				++this->dwSkyX;
				flag_weather_refresh();
			}
		}
	}

	//3. Fog.
	if (this->bFog)
	{
		//Update fog "wind" velocity.
		this->fFogVX += fRAND_MID(0.001f);
		this->fFogVY += fRAND_MID(0.001f);

		//Speed limit.
		if (this->fFogVX > fMaxVelocity) this->fFogVX = fMaxVelocity;
		else if (this->fFogVX < -fMaxVelocity) this->fFogVX = -fMaxVelocity;
		if (this->fFogVY > fMaxVelocity) this->fFogVY = fMaxVelocity;
		else if (this->fFogVY < -fMaxVelocity) this->fFogVY = -fMaxVelocity;

		this->fFogX += this->fFogVX;
		this->fFogY += this->fFogVY;

		if ((int)this->fFogX != (int)this->fFogOldX ||
				(int)this->fFogY != (int)this->fFogOldY)
		{
			//Fog has noticeably moved -- redraw.
			this->fFogOldX = this->fFogX;
			this->fFogOldY = this->fFogY;
			flag_weather_refresh();
		}
	}

	//4. Clouds and cloud shadows.
	if (this->bClouds || (this->bSunlight && !this->bSkyVisible))
	{
		//Clouds always move along at same speed.
		static const float fCloudVelocity = fMaxVelocity;

		//Update direction of wind.
		this->fCloudAngle += fRAND_MID(0.02f);

		this->fCloudX += cos(this->fCloudAngle) * fCloudVelocity;
		this->fCloudY += sin(this->fCloudAngle) * fCloudVelocity;

		if ((int)this->fCloudX != (int)this->fCloudOldX ||
				(int)this->fCloudY != (int)this->fCloudOldY)
		{
			//Clouds have noticeably moved -- redraw.
			this->fCloudOldX = this->fCloudX;
			this->fCloudOldY = this->fCloudY;
			flag_weather_refresh();
		}
	}

	if (this->bAllDirty)
	{
		//Entire room is being refreshed -- don't do anything here.
		this->redrawingRowForWeather = 0;
		this->need_to_update_room_weather = false;
		this->time_of_last_weather_render = dwNow;
	} else if (bPlayerIsAlive) {
		//Refresh weather periodically as requested.
		if (!this->redrawingRowForWeather && this->need_to_update_room_weather)  {
			static const Uint32 MIN_TIME_BETWEEN_WEATHER_RENDERS = 50; //ms
			if (dwNow >= this->time_of_last_weather_render + MIN_TIME_BETWEEN_WEATHER_RENDERS) {
				this->redrawingRowForWeather = CDrodBitmapManager::DISPLAY_ROWS;
				this->time_of_last_weather_render = dwNow;
				this->need_to_update_room_weather = false;
			}
		}

		//As environment slowly changes, update a bit of the room each frame to reflect changes.
		//This keeps the frame rate from slowing too much on older systems.
		if (this->redrawingRowForWeather) {
			int wRowsRedrawnPerFrame = 2 * g_pTheBM->eyeCandy;
			ASSERT(wRowsRedrawnPerFrame > 0);

			//When redrawing more than half of the room, it will take two updates to draw the whole room.
			//So, just draw half of the room each update for more perceived smoothness.
			if (wRowsRedrawnPerFrame > HALF_ROWS && wRowsRedrawnPerFrame < (int)CDrodBitmapManager::DISPLAY_ROWS) {
				wRowsRedrawnPerFrame = HALF_ROWS;
			}

			//draw top-down
			const UINT current_row = this->wShowRow + CDrodBitmapManager::DISPLAY_ROWS - this->redrawingRowForWeather;
			DirtyTileRect(this->wShowCol, current_row,
					CDrodBitmapManager::DISPLAY_COLS-1, current_row + wRowsRedrawnPerFrame - 1);

			if (this->bSkyVisible) {
				//Room tiles reflecting sky must be re-rendered
				RenderRoom(this->wShowCol, current_row,
						CDrodBitmapManager::DISPLAY_COLS, wRowsRedrawnPerFrame,
						false);
			}

			if (wRowsRedrawnPerFrame >= this->redrawingRowForWeather)
				this->redrawingRowForWeather = 0; //done
			else
				this->redrawingRowForWeather -= wRowsRedrawnPerFrame; //partial update -- do more next time
		}
	}
}

void CRoomWidget::flag_weather_refresh()
{
	this->need_to_update_room_weather = true;
}

//*****************************************************************************
void CRoomWidget::AddObstacleShadowMask(const UINT wCol, const UINT wRow)
//Add obstacle's tile shadow masks to set of pending masks.
{
	const BYTE tParam = this->pRoom->GetTParam(wCol, wRow);
	const BYTE obType = calcObstacleType(tParam);
	if (!obType || !bObstacleLeft(tParam) || !bObstacleTop(tParam))
		return; //this isn't the top-left corner of an obstacle -- do nothing

	//Each obstacle casts a shadow mask 1 tile larger than its size,
	//starting at its upper-left corner.
	UINT wObSizeIndex, wXPos, wYPos, wOTile;
	GetObstacleStats(this->pRoom, wCol, wRow, wObSizeIndex, wXPos, wYPos);
	ASSERT(wXPos == 0);
	ASSERT(wYPos == 0);
	const UINT wObIndex = obstacleIndices[obType][wObSizeIndex];
	ASSERT(wObIndex);
	const UINT wXDim = obstacleDimensions[wObIndex][0];
	const UINT wYDim = obstacleDimensions[wObIndex][1];
	ASSERT(wXDim);
	ASSERT(wYDim);

	const UINT wOTileOrigin = this->pRoom->GetOSquare(wCol, wRow);
	UINT wSX, wSY, x, y, wTileIndex;
	y = wRow;
	for (wSY=0; wSY<=wYDim; ++wSY, ++y)
	{
		x = wCol;
		wTileIndex = this->pRoom->ARRAYINDEX(x,y);
		for (wSX=0; wSX<=wXDim; ++wSX, ++x, ++wTileIndex)
		{
			//Shadow is cast if (x,y) is part of the current obstacle
			//(i.e. its NxN square) OR shows shadows.
			if (this->pRoom->IsValidColRow(x,y))
			{
				wOTile = this->pRoom->GetOSquare(x,y);
				if ((wSX<wXDim && wSY<wYDim) ||
					//if obstacle is placed on wall, it casts shadow everywhere
					bObstacleCastsShadowFrom(wOTileOrigin,wOTile))
				{
					TileImages& ti = this->pTileImages[wTileIndex];
					ti.shadowMasks.push_back(
							obstacleShadowTile[wObIndex][wSX][wSY]);
					ti.dirty = 1;
				}
			}
		}
	}
}

//*****************************************************************************
void CRoomWidget::BoundsCheckRect(
//Performs bounds checking on a room rectangle.
//
//Params:
	int &wCol, int &wRow,         //(in/out)  Top-left tile.
	int &wWidth, int &wHeight)    //(in/out)  dimensions, in tiles
const
{
	ASSERT(wWidth > 0 && wHeight > 0);  //avoid wasteful calls
	ASSERT(wCol + wWidth > 0 && wRow + wHeight > 0);
	//Room dimensions.
	const int wRW = this->pRoom->wRoomCols;
	const int wRH = this->pRoom->wRoomRows;
	ASSERT(wCol < wRW && wRow < wRH);

	//Perform bounds checking.
	if (wCol < 0) {wWidth += wCol; wCol = 0;}
	if (wRow < 0) {wHeight += wRow; wRow = 0;}
	if (wCol + wWidth >= wRW) {wWidth = wRW - wCol;}
	if (wRow + wHeight >= wRH) {wHeight = wRH - wRow;}
}

//*****************************************************************************
void CRoomWidget::BAndWRect(
//Sets a rectangle of tiles in room to black-and-white.
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	int wCol, int wRow,        //(in)   Top-left tile.
	int wWidth, int wHeight)   //(in)   dimensions, in tiles
{
	BoundsCheckRect(wCol,wRow,wWidth,wHeight);

	g_pTheBM->BAndWRect(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE,
			wWidth * CX_TILE, wHeight * CY_TILE, pDestSurface);
}

void CRoomWidget::BandWTile(
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	int wCol, int wRow)        //(in)   Tile
{
	g_pTheBM->BAndWTile(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DarkenRect(
//Darkens a rectangle of tiles in room by given percent.
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const float fLightPercent, //(in)   % of color value to retain
	int wCol, int wRow,        //(in)   Top-left tile.
	int wWidth, int wHeight)   //(in)   dimensions, in tiles
{
	BoundsCheckRect(wCol,wRow,wWidth,wHeight);

	g_pTheBM->DarkenRect(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE,
			wWidth * CX_TILE, wHeight * CY_TILE, fLightPercent, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::NegativeTile(
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	int wCol, int wRow)        //(in)   Tile
{
	g_pTheBM->NegativeTile(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::ShadeRect(
//Shades a rectangle of tiles in room with given color.
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const SURFACECOLOR &Color, //(in)   Color to shade with
	int wCol, int wRow,        //(in)   Top-left tile.
	int wWidth, int wHeight)   //(in)   dimensions, in tiles
{
	BoundsCheckRect(wCol,wRow,wWidth,wHeight);

	g_pTheBM->ShadeRect(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE,
			wWidth * CX_TILE, wHeight * CY_TILE, Color, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::SepiaTile(
//
//Params:
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	int wCol, int wRow)        //(in)   Tile
{
	g_pTheBM->SepiaTile(this->x + wCol * CX_TILE, this->y + wRow * CY_TILE, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DirtyTileRect(
//Mark all tiles between these two points as dirty.
//
//Params:
	const int x1, const int y1, const int x2, const int y2)  //(in)
{
	//Find min/max coords.
	int xMin = min(x1,x2);
	int xMax = max(x1,x2);
	int yMin = min(y1,y2);
	int yMax = max(y1,y2);

	//Only dirty tiles in visible room area.
	if (xMin < 0) xMin = 0;
	if (yMin < 0) yMin = 0;
	if (xMax >= static_cast<int>(CDrodBitmapManager::DISPLAY_COLS)) xMax = CDrodBitmapManager::DISPLAY_COLS-1;
	if (yMax >= static_cast<int>(CDrodBitmapManager::DISPLAY_ROWS)) yMax = CDrodBitmapManager::DISPLAY_ROWS-1;

	this->pLastLayerEffects->DirtyTilesInRect(xMin,yMin,xMax,yMax);
}

//*****************************************************************************
void CRoomWidget::DrawDamagedMonsters(
//Paint monsters whose tiles have been repainted.
//Also check whether monster is raised up and tile above is dirty.
//This signifies the monster's image has been clipped on top
//and must be repainted.
//
//Params:
	SDL_Surface *pDestSurface, const CharacterDisplayMode eDisplayMode)
{
	ASSERT(!this->bAllDirty);

	TileImages *pTI;
	UINT wSX, wSY;
	vector<CMonster*> drawnMonsters;
	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (!DisplayMonster(pMonster, eDisplayMode))
			continue;

		bool bDrawMonster = false;

		if ((pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2) &&
				this->pCurrentGame) //not in room editor
		{
			//Redraw Slayer's seeker wisp each turn
			bDrawMonster = true;
		} else if (bIsSerpent(pMonster->wType)) {
			//Redraw dirty serpent pieces.  The head only will be redrawn below as applicable.
			for (MonsterPieces::const_iterator pieceIt = pMonster->Pieces.begin();
					pieceIt != pMonster->Pieces.end(); ++pieceIt)
			{
				const CMonsterPiece& piece = **pieceIt;
				ASSERT(this->pRoom->IsValidColRow(piece.wX, piece.wY));
				pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX, piece.wY);
				if (pTI->dirty || pTI->damaged)
				{
					//Copied from DrawSerpentBody().
					const TileImageBlitParams blit(piece.wX, piece.wY, GetTileImageForSerpentPiece(pMonster->wType, piece.wTileNo));
					DrawTileImage(blit, pDestSurface);
				}
			}
		} else if (pMonster->wType == M_GENTRYII) {
			//If any pieces of a gentryii body are dirty, including corners adjacent to a diagonal,
			//redraw the respective link.
			UINT wPrevX = pMonster->wX;
			UINT wPrevY = pMonster->wY;
			MonsterPieces::const_iterator pieces_end = pMonster->Pieces.end();
			for (MonsterPieces::const_iterator pieceIt = pMonster->Pieces.begin();
					pieceIt != pieces_end; ++pieceIt)
			{
				const CMonsterPiece& piece = **pieceIt;
				bool bDrawLink = false;

				ASSERT(this->pRoom->IsValidColRow(piece.wX, piece.wY));
				pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX, piece.wY);
				if (pTI->dirty || pTI->damaged) {
					bDrawLink = true;
				} else {
					//Adjacent corner to diagonal check.
					const int dx = piece.wX - wPrevX;
					const int dy = piece.wY - wPrevY;
					if (dx && dy) {
						if (dx > 0) {
							pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX-1, piece.wY);
							if (pTI->dirty || pTI->damaged)
								bDrawLink = true;
						} else {
							pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX+1, piece.wY);
							if (pTI->dirty || pTI->damaged)
								bDrawLink = true;
						}
						if (dy > 0) {
							pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX, piece.wY-1);
							if (pTI->dirty || pTI->damaged)
								bDrawLink = true;
						} else {
							pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX, piece.wY+1);
							if (pTI->dirty || pTI->damaged)
								bDrawLink = true;
						}
					}
				}

				if (bDrawLink)
					DrawGentryiiChainLink(pieceIt, pieces_end, wPrevX, wPrevY, pDestSurface, false);

				wPrevX = piece.wX;
				wPrevY = piece.wY;
			}
		} else if (pMonster->IsLongMonster()) {
			//If any pieces of a large monster are dirty, redraw the entire monster.
			for (MonsterPieces::const_iterator pieceIt = pMonster->Pieces.begin();
					pieceIt != pMonster->Pieces.end(); ++pieceIt)
			{
				const CMonsterPiece& piece = **pieceIt;
				ASSERT(this->pRoom->IsValidColRow(piece.wX, piece.wY));
				pTI = this->pTileImages + this->pRoom->ARRAYINDEX(piece.wX, piece.wY);
				if (pTI->dirty || pTI->damaged)
				{
					bDrawMonster = true;
					break;
				}
			}
		}
		//Check whether monster's sword square is dirty also.
		else if (pMonster->GetSwordCoords(wSX, wSY)) {
			if (IS_COLROW_IN_DISP(wSX, wSY))
			{
				ASSERT(this->pRoom->IsValidColRow(wSX, wSY));
				pTI = this->pTileImages + this->pRoom->ARRAYINDEX(wSX, wSY);
				if (pTI->dirty || pTI->damaged) {
					bDrawMonster = true;
				} else if (wSY > this->wShowRow && DrawRaised(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY)))
				{
					//If it's raised and the monster above it is being redrawn, then it must be too.
					pTI = this->pTileImages + this->pRoom->ARRAYINDEX(wSX, wSY-1);
					if (pTI->dirty || pTI->damaged)
						bDrawMonster = true;
				}
			}
		}

		if (!bDrawMonster)
		{
			ASSERT(this->pRoom->IsValidColRow(pMonster->wX, pMonster->wY));
			pTI = this->pTileImages + this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY);
			if (pTI->dirty || pTI->damaged) {
				bDrawMonster = true;
			} else if (pMonster->wY > this->wShowRow && DrawRaised(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY)))
			{
				//If it's raised and the monster above it is being redrawn, then it must be too.
				pTI = this->pTileImages + this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY-1);
				if (pTI->dirty || pTI->damaged)
					bDrawMonster = true;
			}
		}

		if (bDrawMonster)
		{
			DrawMonster(pMonster, this->pRoom, pDestSurface, false, false, false);
			drawnMonsters.push_back(pMonster);
		}
	}

	DrawSwordsFor(drawnMonsters, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DrawDamagedMonsterSwords(SDL_Surface *pDestSurface)
{
	for (const CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		UINT wSX, wSY;
		if (DrawingSwordFor(pMonster) && pMonster->GetSwordCoords(wSX, wSY))
		{
			if (IS_COLROW_IN_DISP(wSX, wSY))
			{
				if (!NeedsSwordRedrawing(pMonster))
					continue;

				ASSERT(this->pRoom->IsValidColRow(wSX, wSY));
				const TileImages& ti = this->pTileImages[this->pRoom->ARRAYINDEX(wSX, wSY)];
				if (ti.dirty || ti.damaged) {
					UINT wXOffset = 0, wYOffset = 0;
					JitterBoundsCheck(wSX, wSY, wXOffset, wYOffset);
					TileImageBlitParams blit(wSX, wSY, 0, wXOffset, wYOffset, false,
						DrawRaised(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY)));
					DrawSwordFor(pMonster, pMonster->GetIdentity(), blit, pDestSurface);
				}
			}
		}
	}
}

//*****************************************************************************
bool CRoomWidget::DisplayMonster(const CMonster* pMonster, const CharacterDisplayMode eDisplayMode) const
{
	bool bDisplay = eDisplayMode == CDM_Normal;
	if (pMonster->wType == M_CHARACTER)
	{
		const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
		if (pCharacter->DisplayGhostImage()) {
			bDisplay = (eDisplayMode == CDM_GhostFloor && pCharacter->IsGhostImageFloor()) ||
				(eDisplayMode == CDM_GhostOverhead && pCharacter->IsGhostImageOverhead());
		}
	}
	return bDisplay;
}

//*****************************************************************************
void CRoomWidget::DrawPlatformsAndTLayer(
//Draws platforms in the room.
	SDL_Surface *pDestSurface,
	const bool bEditor,           //(in) [default=false]
	const bool bMoveInProgress)   //(in) [default=false]
{
	ASSERT(pDestSurface);

	const float fLightLevel = fRoomLightLevel[this->wDark];
	const bool bAddLight = IsLightingRendered();

	CCoordIndex tilesDrawn(this->pRoom->wRoomCols, this->pRoom->wRoomRows);
	vector<TileImageBlitParams> lightingBlits;

	// This is a hack to work around the fact that tiles are generally drawn with room's light level
	// But because we can have image overlays, we need to apply the darkness during the lighting pass
	// after the effects are drawn
	const float fOldLightLevel = CBitmapManager::fLightLevel;
	CBitmapManager::fLightLevel = 1.0;

	//Render each platform.
	ASSERT(this->pRoom);
	for (vector<CPlatform*>::const_iterator platformIter = this->pRoom->platforms.begin();
			platformIter != this->pRoom->platforms.end(); ++platformIter)
	{
		const CPlatform& platform = *(*platformIter);
		const bool bPlatformMoved = platform.xDelta || platform.yDelta;
		const bool bPlatformAnimating = bMoveInProgress && bPlatformMoved;
		const bool bRedrawAll = this->bAllDirty || bPlatformAnimating || bEditor;

		//Calculate animation offset (in pixels).
		UINT wXOffset = 0, wYOffset = 0;
		if (this->dwMovementStepsLeft)
		{
			wXOffset = -platform.xDelta * this->dwMovementStepsLeft;
			wYOffset = -platform.yDelta * this->dwMovementStepsLeft;
		}

		CCoordSet tiles;
		platform.GetTiles(tiles);

		//Redraw tiles needing to be repainted.
		CCoordSet::const_iterator tileIter;
		for (tileIter = tiles.begin(); tileIter != tiles.end(); ++tileIter)
		{
			UINT wX = tileIter->wX, wY = tileIter->wY;
			const TileImages& ti = this->pTileImages[this->pRoom->ARRAYINDEX(wX,wY)];
			if (bRedrawAll || ti.damaged)
			{
				const UINT oTile = this->pRoom->GetOSquare(wX, wY);
				UINT wTileImageNo = GetTileImageForTileNo(oTile);
				if (wTileImageNo == CALC_NEEDED)
				{
					
					wTileImageNo = CalcTileImageFor(this->pRoom, oTile, wX, wY);
					TileImageBlitParams blit(wX, wY, wTileImageNo, wXOffset, wYOffset,
						bPlatformAnimating); // || wXOffset || wYOffset); -- only needed if platform has jitter independent of movement animation
					if (oTile == T_PLATFORM_W) {
						blit.bCastShadowsOnTop = false;
						// Platforms over water have their lighting applied when drawing T-Layer items
					} else {
						//Platform over pit.
						//Add shadow and light on platform after the platform and o-layer effects are drawn.
						lightingBlits.push_back(blit);
					}
					DrawTileImageWithoutLight(blit, pDestSurface);
					tilesDrawn.Add(wX,wY);
					//tiles being moved off of need to have items redrawn too
					tilesDrawn.Add(wX - platform.xDelta, wY - platform.yDelta);
					if (platform.xDelta && platform.yDelta) {
						//diagonal movement -- also redraw tiles that platform corners are passing under
						tilesDrawn.Add(wX - platform.xDelta, wY);
						tilesDrawn.Add(wX, wY - platform.yDelta);
					}
				}
				//otherwise: we're cutting-and-pasting this platform in the editor, so nothing special should be drawn at present
			}
		}
	}

	this->pOLayerEffects->DrawEffects();

	CBitmapManager::fLightLevel = fOldLightLevel;

	//Apply lighting to pit platforms
	//
	//Maintain record of where moving platform tile light+shadow has been applied.
	//When pit darkness is applied, do not draw it on any of these regions.
	t_PitMasks pitMasks;
	for (vector<TileImageBlitParams>::iterator iter = lightingBlits.begin();
		iter != lightingBlits.end(); ++iter)
	{
		const TileImageBlitParams& blit = *iter;
		//Shade platform according to overall room light level.
		g_pTheBM->DarkenRect(this->x + blit.wCol * CX_TILE + blit.wXOffset,
			this->y + blit.wRow * CY_TILE + blit.wYOffset,
			CX_TILE, CY_TILE, CBitmapManager::fLightLevel, pDestSurface);

		//Apply overhead darkness and lightmap.
		DrawTileLight(blit, pDestSurface);

		if (bAddLight)
			AddPlatformPitMasks(blit, pitMasks);
	}

	DrawTLayerTiles(tilesDrawn, pitMasks, pDestSurface, fLightLevel, bAddLight, bEditor);
}

//*****************************************************************************
void CRoomWidget::AddPlatformPitMasks(
	const TileImageBlitParams& blit,
	t_PitMasks& pitMasks) //(in/out)
{
	if (!blit.wXOffset && !blit.wYOffset)
		return; //no masking needed

	//Pit under moving platform is partially occluded.
	//Track either two or four pit tiles that this platform tile is occluding.
	pitMasks[ROOMCOORD(blit.wCol, blit.wRow)].push_back(
		TweeningTileMask(blit.wTileImageNo, blit.wXOffset, blit.wYOffset));

	if (blit.wXOffset) {
		if (int(blit.wXOffset) < 0) { //moving from left
			pitMasks[ROOMCOORD(blit.wCol - 1, blit.wRow)].push_back(
				TweeningTileMask(blit.wTileImageNo, CX_TILE + int(blit.wXOffset), blit.wYOffset));
		}
		else { //moving from right
			pitMasks[ROOMCOORD(blit.wCol + 1, blit.wRow)].push_back(
				TweeningTileMask(blit.wTileImageNo, int(blit.wXOffset) - CX_TILE, blit.wYOffset));
		}

		if (blit.wYOffset) {
			if (int(blit.wXOffset) < 0) { //moving from left
				if (int(blit.wYOffset) < 0) { //moving from above
					pitMasks[ROOMCOORD(blit.wCol - 1, blit.wRow - 1)].push_back(
						TweeningTileMask(blit.wTileImageNo, CX_TILE + int(blit.wXOffset), CY_TILE + int(blit.wYOffset)));
				}
				else { //moving from below
					pitMasks[ROOMCOORD(blit.wCol - 1, blit.wRow + 1)].push_back(
						TweeningTileMask(blit.wTileImageNo, CX_TILE + int(blit.wXOffset), int(blit.wYOffset) - CY_TILE));
				}
			}
			else { //moving from right
				if (int(blit.wYOffset) < 0) { //moving from above
					pitMasks[ROOMCOORD(blit.wCol + 1, blit.wRow - 1)].push_back(
						TweeningTileMask(blit.wTileImageNo, int(blit.wXOffset) - CX_TILE, CY_TILE + int(blit.wYOffset)));
				}
				else { //moving from below
					pitMasks[ROOMCOORD(blit.wCol + 1, blit.wRow + 1)].push_back(
						TweeningTileMask(blit.wTileImageNo, int(blit.wXOffset) - CX_TILE, int(blit.wYOffset) - CY_TILE));
				}
			}
		}
	}

	if (blit.wYOffset) {
		if (int(blit.wYOffset) < 0) { //moving from above
			pitMasks[ROOMCOORD(blit.wCol, blit.wRow - 1)].push_back(
				TweeningTileMask(blit.wTileImageNo, blit.wXOffset, CY_TILE + int(blit.wYOffset)));
		}
		else { //moving from below
			pitMasks[ROOMCOORD(blit.wCol, blit.wRow + 1)].push_back(
				TweeningTileMask(blit.wTileImageNo, blit.wXOffset, int(blit.wYOffset) - CY_TILE));
		}
	}
}

//*****************************************************************************
void CRoomWidget::DrawTLayerTiles(
	const CCoordIndex& tiles,
	const t_PitMasks& pitMasks,
	SDL_Surface *pDestSurface,
	const float fLightLevel, const bool bAddLight, const bool bEditor)
{
	const UINT wStartPos = this->wShowRow * this->pRoom->wRoomCols + this->wShowCol;
	const UINT wRowOffset = this->pRoom->wRoomCols - CDrodBitmapManager::DISPLAY_COLS;
	TileImages *pTI = this->pTileImages + wStartPos;

	ASSERT(this->movingTLayerObjectsToRender.empty());

	for (UINT wY = this->wShowRow; wY < CDrodBitmapManager::DISPLAY_ROWS; ++wY, pTI += wRowOffset)
	{
		for (UINT wX = this->wShowCol; wX < CDrodBitmapManager::DISPLAY_COLS; ++wX, ++pTI)
		{
			if (this->bAllDirty || pTI->damaged || tiles.Exists(wX, wY))
			{
				//Get tile-specific info.
				UINT wOTileNo = this->pRoom->GetOSquare(wX, wY);

				//Determine this tile's darkness.
				const float fDark = fLightLevel * GetOverheadDarknessAt(wX, wY);

				int nX = this->x + wX * CX_TILE;
				int nY = this->y + wY * CY_TILE;
				const UINT tileIndex = this->pRoom->ARRAYINDEX(wX, wY);

				LIGHTTYPE *psL = this->lightMaps.psDisplayedLight + tileIndex * wLightValuesPerTile;

				const vector<TweeningTileMask>* pPitMasks = NULL;
				if (bAddLight) {
					t_PitMasks::const_iterator it = pitMasks.find(ROOMCOORD(wX, wY));
					if (it != pitMasks.end())
						pPitMasks = &(it->second);
				}

				DrawTLayerTile(wX, wY, nX, nY, pDestSurface,
						wOTileNo, this->pTileImages[tileIndex], psL,
						fDark, bAddLight, bEditor, pPitMasks);

				if (tiles.Exists(wX, wY))
					this->pTileImages[tileIndex].dirty = 1;
			}
		}
	}

	//Render all moving t-layer objects at this point
	for (CCoordSet::const_iterator it=this->movingTLayerObjectsToRender.begin();
			it!=this->movingTLayerObjectsToRender.end(); ++it)
	{
		const ROOMCOORD& coord = *it;
		const UINT tileIndex = this->pRoom->ARRAYINDEX(coord.wX, coord.wY);
		const UINT tileImage = this->pTileImages[tileIndex].t;
		const RoomObject *pObj = this->pRoom->GetTObject(tileIndex);
		ASSERT(pObj);
		const UINT wXOffset = (pObj->wPrevX - coord.wX) * this->dwMovementStepsLeft;
		const UINT wYOffset = (pObj->wPrevY - coord.wY) * this->dwMovementStepsLeft;
		TileImageBlitParams blit(coord.wX, coord.wY, tileImage, wXOffset, wYOffset, true);
		blit.appliedDarkness = 1; //apply lighting the same as for stationary room tiles and objects
		DrawTileImage(blit, pDestSurface);
		//Absolutely required, otherwise on slow repeat speed a quick undo followed by
		// action can cause an object to disappear, because its first blit will be entirely
		// in the previous position, causing it to not be drawn nor animated on subsqeuent frames
		this->pTileImages[tileIndex].dirty = true;
	}
	this->movingTLayerObjectsToRender.clear();
}

//*****************************************************************************
void CRoomWidget::DrawMonsters(
//Draws monsters not on the player -- this case is handled separately.
//
//Params:
	CMonster *const pMonsterList, //(in)   Monsters to draw.
	SDL_Surface *pDestSurface,
	const bool bActionIsFrozen,   //(in)   Whether action is currently stopped.
	const bool bMoveInProgress)   //(in)   [default=false]
{
	CMonster *pMonster;

	//Draw "ghost" (floor) NPCs first.
	for (pMonster = pMonsterList; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			if (pCharacter->DisplayGhostImage() && pCharacter->IsGhostImageFloor()) {
				DrawMonster(pMonster, this->pRoom, pDestSurface, bActionIsFrozen, bMoveInProgress);
			}
		}
	}

	//Get player info.
	UINT wPX = UINT(-1), wPY = UINT(-1);
	bool bPlayerIsDying = false;
	if (this->pCurrentGame)
	{
		bPlayerIsDying = this->pCurrentGame->IsPlayerDying();
		this->pCurrentGame->GetSwordsman(wPX, wPY);
	}

	vector<CMonster*> drawnMonsters;
	for (pMonster = pMonsterList; pMonster; pMonster = pMonster->pNext)
	{
		//Ghost characters are draw above or below
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->DisplayGhostImage())
				continue;
		}

		//Draw monster if it is not involved in a death sequence.
		if (!bPlayerIsDying || (!IsMonsterInvolvedInDeath(pMonster) && pMonster->bAlive))
		{
			DrawMonster(pMonster, this->pRoom, pDestSurface, bActionIsFrozen, bMoveInProgress);
			drawnMonsters.push_back(pMonster);
		}
	}

	//Redraw monster swords to ensure no other monster is being drawn on top of them.
	if (!(bMoveInProgress || this->dwMovementStepsLeft))
	{
		DrawSwordsFor(drawnMonsters, pDestSurface);
	}
}

void CRoomWidget::DrawSwordsFor(const vector<CMonster*>& drawnMonsters, SDL_Surface *pDestSurface)
{
	for (vector<CMonster*>::const_iterator monster=drawnMonsters.begin();
		monster != drawnMonsters.end(); ++monster)
	{
		const CMonster *pMonster = *monster;

		UINT wSX, wSY;
		if (pMonster->GetSwordCoords(wSX, wSY)) {
			if (!NeedsSwordRedrawing(pMonster))
				continue;

			const bool draw_raised = DrawRaised(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY));
			UINT wXOffset = 0, wYOffset = 0;
			JitterBoundsCheck(wSX, wSY, wXOffset, wYOffset);
			TileImageBlitParams blit(wSX, wSY, 0, wXOffset, wYOffset, false, draw_raised);
			DrawSwordFor(pMonster, pMonster->GetIdentity(), blit, pDestSurface);
		}
	}
}

//*****************************************************************************
bool CRoomWidget::IsShowingBetterVision() const
{
	return this->pRoom && this->pRoom->bBetterVision;
}

//*****************************************************************************
bool CRoomWidget::NeedsSwordRedrawing(const CMonster *pMonster) const
{
	//These monsters might always animate,
	//so don't need redrawing to fix damaged screen areas in these cases.
	if (pMonster->wType == M_TEMPORALCLONE)
		return !IsTemporalCloneAnimated();

	// Hiding clones are redrawn every turn
	if (pMonster->wType == M_CLONE)
		return !pMonster->IsHiding();

	return true;
}

//*****************************************************************************
void CRoomWidget::AnimateMonsters()
//Randomly change monsters' animation frame.
{
	if (!this->bAnimateMoves || this->puzzleModeOptions.GetHideAnimations())
		return;

	//Animate monsters in real time.
	const Uint32 dwNow=SDL_GetTicks();
	Uint32 dwTimeElapsed = dwNow - this->dwLastAnimationFrame;
	if (dwTimeElapsed==0)
		dwTimeElapsed=1;
	const Uint32 dwRandScalar=MONSTER_ANIMATION_DELAY * 1000 / dwTimeElapsed;

	static const Uint32 BEETHRO_BREATHING_RATE = 400; //ms
	const bool bAnimateBeethro = dwNow - this->dwLastBeethroAnimation >= BEETHRO_BREATHING_RATE;
	if (bAnimateBeethro) {
		this->dwLastBeethroAnimation = dwNow;
		++this->dwBeethroAnimationFrame;
	}

	static const Uint32 MONSTER_BREATHING_RATE = 300; //ms
	const bool bAnimateMonster = dwNow - this->dwLastMonsterAnimation >= MONSTER_BREATHING_RATE;
	if (bAnimateMonster)
		this->dwLastMonsterAnimation = dwNow;

	const bool bAlpha = g_pTheBM->bAlpha;
	UINT wSX, wSY;
	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL;
			pMonster = pMonster->pNext)
	{
		const bool bCyclingMonsterAnimation = IsMonsterTypeAnimated(pMonster->wType);
		if (bCyclingMonsterAnimation || (!bCyclingMonsterAnimation && RAND(dwRandScalar) == 0))
		{
			AnimateMonster(pMonster);
		}
		//Redraw transparent monsters each frame.
		else if (this->pCurrentGame)
		{
			//Jittered entities must always be redrawn.
			TileImages& ti = this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)];
			if (this->jitterInfo.Exists(pMonster->wX, pMonster->wY)) {
				ti.dirty = 1;
			} else if ((pMonster->eMovement == WALL && bAlpha) || pMonster->wType == M_SPIDER || pMonster->wType == M_FLUFFBABY) {
				//Transparent monsters.
				ti.dirty = 1;
			} else if (pMonster->wType == M_CHARACTER) {
				//Custom animated NPCs
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				if (pCharacter->pCustomChar)
				{
					if (pCharacter->pCustomChar->animationSpeed) {
						ti.dirty = 1;
						if (pMonster->wY > 0 && DrawRaised(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY)))
							this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY-1)].dirty = 1;
					}
				} else if (bAnimateBeethro) {
					if (pCharacter->wIdentity == M_BEETHRO) {
						ti.dirty = 1;
					} else if (pCharacter->wIdentity == M_CLONE || pCharacter->wIdentity == M_TEMPORALCLONE) {
						const UINT wIdentity = pCharacter->GetResolvedIdentity();
						if (wIdentity == M_CLONE || wIdentity == M_BEETHRO)
							ti.dirty = 1;
					}
				}
			} else if (bAnimateBeethro && pMonster->wType == M_CLONE) { //temporal clone is already animated each frame, so don't need to dirty it here
				const CArmedMonster* pClone = DYN_CAST(const CArmedMonster*, const CMonster*, pMonster);
				const UINT wIdentity = pClone->GetIdentity();
				if (wIdentity == M_CLONE || wIdentity == M_BEETHRO)
					ti.dirty = 1;
			}
		}

		//Always redraw all changing parts of large and sworded monsters in fog.
		if (this->cFogLayer > 1)
		{
			TileImages& ti = this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)];
			if (pMonster->IsLongMonster() && !bIsSerpent(pMonster->wType))
			{
				ti.dirty = 1;
				for (MonsterPieces::const_iterator piece = pMonster->Pieces.begin();
						piece != pMonster->Pieces.end(); ++piece)
					this->pTileImages[this->pRoom->ARRAYINDEX((*piece)->wX, (*piece)->wY)].dirty = 1;
			}
			if (pMonster->GetSwordCoords(wSX, wSY) && this->pRoom->IsValidColRow(wSX, wSY))
			{
				ti.dirty = 1;
				this->pTileImages[this->pRoom->ARRAYINDEX(wSX, wSY)].dirty = 1;
			}
		}
	}

	//Animate non-Beethro player role.
	if (this->pCurrentGame)
	{
		CSwordsman& player = this->pCurrentGame->swordsman;
		if (player.wAppearance == M_BEETHRO) {
			if (bAnimateBeethro)
				this->pTileImages[this->pRoom->ARRAYINDEX(player.wX, player.wY)].dirty = 1;
		}
		else if (!bIsSmitemaster(player.wAppearance) && RAND(dwRandScalar) == 0)
		{
			SwitchAnimationFrame(player.wX, player.wY);
			this->pTileImages[this->pRoom->ARRAYINDEX(player.wX, player.wY)].dirty = 1;
		}
	}

	this->dwLastAnimationFrame=dwNow;
}

//*****************************************************************************
void CRoomWidget::AnimateMonster(CMonster* pMonster)
{
	if (IsMonsterTypeAnimated(pMonster->wType)) {
		AdvanceAnimationFrame(pMonster);
	} else {
		SwitchAnimationFrame(pMonster->wX, pMonster->wY);
	}

	this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)].dirty = 1;

	// Also dirty the tile with the weapon, to ensure it's properly redrawn
	const CArmedMonster* pArmedMonster = dynamic_cast<CArmedMonster*>(pMonster);
	if (pArmedMonster
		&& pArmedMonster->HasSword()
		&& this->pRoom->IsValidColRow(pArmedMonster->GetWeaponX(), pArmedMonster->GetWeaponY())
	)
		this->pTileImages[this->pRoom->ARRAYINDEX(pArmedMonster->GetWeaponX(), pArmedMonster->GetWeaponY())].dirty = 1;

	//Redraw all changing parts of large monsters.
	if (pMonster->IsLongMonster() && !bIsSerpentOrGentryii(pMonster->wType))
	{
		for (MonsterPieces::const_iterator piece = pMonster->Pieces.begin();
				piece != pMonster->Pieces.end(); ++piece)
			this->pTileImages[this->pRoom->ARRAYINDEX((*piece)->wX, (*piece)->wY)].dirty = 1;
	}
}
//*****************************************************************************
void CRoomWidget::BetterVisionQuery()
//Display visual effects for vision power up.
{
	//Evil eye gazes are seen.
	this->bRequestEvilEyeGaze = IsShowingBetterVision() || this->puzzleModeOptions.GetShowEvilEyeBeams();
	this->bRequestSpiderVisibility = IsShowingBetterVision() || this->puzzleModeOptions.GetShowSpiders();
	this->bRequestTranslucentTar = IsShowingBetterVision() || this->puzzleModeOptions.bIsEnabled;

	//Remove old persistent evil eye gazes.
	//Note that normal wake up gaze effects are displayed on the m-layer,
	//not the last layer like these.
	this->pLastLayerEffects->RemoveEffectsOfType(EEVILEYEGAZE);
}

//*****************************************************************************
void CRoomWidget::DrawMonster(
//Draws a monster.
//
//Params:
	const CMonster* pMonster,   //(in) Monster to draw.
	CDbRoom *const pRoom,
	SDL_Surface *pDestSurface,
	const bool bActionIsFrozen, //(in) Whether action is currently stopped.
	const bool bMoveInProgress, //(in) Whether movement animation is not yet completed [default=true]
	const bool bDrawPieces)     //(in) Draw large monster pieces [default=true]
{
	bool bDrawRaised =
			DrawRaised(pRoom->GetOSquare(pMonster->wX, pMonster->wY));
	Uint8 opacity = 255;
	switch (pMonster->wType)
	{
		case M_CITIZEN:
			DrawCitizen(DYN_CAST(const CCitizen*, const CMonster*, pMonster), bDrawRaised, pDestSurface, bMoveInProgress);
			break;
		case M_SLAYER:
		case M_SLAYER2:
			DrawSlayerWisp(pMonster, pDestSurface);
			//FALL-THROUGH
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_GUARD: case M_STALWART: case M_STALWART2:
		case M_TEMPORALCLONE:
			DrawArmedMonster(DYN_CAST(const CArmedMonster*, const CMonster*, pMonster), bDrawRaised, pDestSurface, bMoveInProgress);
			break;
		case M_CHARACTER:
			DrawCharacter(DYN_CAST(const CCharacter*, const CMonster*, pMonster), bDrawRaised, pDestSurface, bMoveInProgress);
			break;
		case M_ROCKGIANT:
			DrawRockGiant(pMonster, bDrawRaised, pDestSurface, bMoveInProgress);
			break;
		case M_SERPENT:
		case M_SERPENTG:
		case M_SERPENTB:
		{
			if (bDrawPieces)
				DrawSerpentBody(pMonster, pDestSurface, bMoveInProgress);

			UINT wXOffset = 0, wYOffset = 0;
			if (this->dwMovementStepsLeft)
			{
				ASSERT(!pMonster->Pieces.empty());
				const CMonsterPiece& piece = *(pMonster->Pieces.back()); //piece after head
				if (piece.wTileNo == T_SNK_EW || piece.wTileNo == T_SNK_NS) //full straight piece
				{
					//Jump the head forward a bit so it's ahead of the first body piece
					//from the beginning of the animation.
					const UINT stepsLeft = this->dwMovementStepsLeft * 19/CX_TILE;

					wXOffset = (pMonster->wPrevX - pMonster->wX) * stepsLeft;
					wYOffset = (pMonster->wPrevY - pMonster->wY) * stepsLeft;
				}
			}
			AddJitterOffset(pMonster->wX, pMonster->wY, wXOffset, wYOffset);

			const UINT wFrame =
				this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)].animFrame;

			const UINT wTileImageNo = GetTileImageForEntity(pMonster->wType, pMonster->wO, wFrame);
			const TileImageBlitParams blit(pMonster->wX, pMonster->wY, wTileImageNo, wXOffset, wYOffset, bMoveInProgress);
			DrawTileImage(blit, pDestSurface);
		}
		break;
		case M_GENTRYII:
		{
			DrawGentryiiChain(pMonster, pDestSurface, bMoveInProgress);

			UINT wXOffset = 0, wYOffset = 0;
			if (this->dwMovementStepsLeft)
			{
				bool smooth_animation = pMonster->Pieces.empty();
				if (!smooth_animation) {
					const CMonsterPiece& piece = *(pMonster->Pieces.front());
					smooth_animation = piece.wX == pMonster->wPrevX &&
							piece.wY == pMonster->wPrevY;
				}
				if (smooth_animation) {
					UINT stepsLeft = this->dwMovementStepsLeft;
					const UINT dx = pMonster->wPrevX - pMonster->wX;
					const UINT dy = pMonster->wPrevY - pMonster->wY;
					if (dx && dy && !pMonster->Pieces.empty()) {
						//On diagonal moves, the diagonal chain attached to the head
						//appears ahead of the head's starting position.
						//Jump the head forward a bit so it's ahead of the chain
						//from the beginning of the animation.
						stepsLeft = stepsLeft * 4 / 5;
					}
					wXOffset = dx * stepsLeft;
					wYOffset = dy * stepsLeft;
				}
			}
			AddJitterOffset(pMonster->wX, pMonster->wY, wXOffset, wYOffset);

			const UINT wFrame =
				this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)].animFrame;

			const UINT wTileImageNo = GetTileImageForEntity(pMonster->wType, pMonster->wO, wFrame);
			const TileImageBlitParams blit(pMonster->wX, pMonster->wY, wTileImageNo, wXOffset, wYOffset, bMoveInProgress);
			DrawTileImage(blit, pDestSurface);
		}
		break;
		default:
		{
			const bool bWading = bIsWadingMonsterType(pMonster->wType) && pMonster->IsWading();
			const UINT animFrame = this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)].animFrame;
			UINT wFrame;
			if (bWading) {
				wFrame = UINT(animFrame ? WADING_FRAME2 : WADING_FRAME);
			} else {
				wFrame = animFrame;
				if (!IsMonsterTypeAnimated(pMonster->wType))
					wFrame = wFrame % ANIMATION_FRAMES;
			}

			//Calculate animation offset.
			UINT wXOffset = 0, wYOffset = 0;
			if (this->dwMovementStepsLeft)
			{
				wXOffset = (pMonster->wPrevX - pMonster->wX) * this->dwMovementStepsLeft;
				wYOffset = (pMonster->wPrevY - pMonster->wY) * this->dwMovementStepsLeft;
			}
			AddJitterOffset(pMonster->wX, pMonster->wY, wXOffset, wYOffset);

			UINT wTileImageNo = GetTileImageForEntity(pMonster->wType, pMonster->wO, wFrame);

			int nAddColor = NO_COLOR_INDEX;
			if (pMonster->eMovement == WALL)
			{
				//Monsters in walls are not raised, and are transparent.
				bDrawRaised = false;
				if (!bActionIsFrozen) //only draw transparent when play is in progress
					opacity = this->ghostOpacity;
			}
			else if (bIsMother(pMonster->wType))
				bDrawRaised = false; //tarstuff is never raised
			else switch (pMonster->wType)
			{
				case M_FLUFFBABY:
					opacity = PUFF_OPACITY;
				break;
				case M_SPIDER:
					if (this->bRequestSpiderVisibility) {
						//display, even when invisible
					} else if (this->pCurrentGame) {
						const CSpider *pSpider = DYN_CAST(const CSpider*, const CMonster*, pMonster);
						if (!pSpider->IsSpiderVisible())
							return; //don't draw spider when invisible

						//spiders are about 35% opaque, more when adjacent to player
						const UINT wDistance = pMonster->DistToSwordsman(false);
						switch (wDistance)
						{
							case 0: case 1: opacity = 190; break;
							case 2: opacity = 145; break;
							case 3: opacity = 135; break;
							case 4: opacity = 125; break;
							case 5: opacity = 110; break;
							default: opacity = 95; break;
						}
					}
				break;
				case M_EYE:
				{
					//Draw active eyes distinctly.
					const CEvilEye *pEye = DYN_CAST(const CEvilEye*, const CMonster*, pMonster);
					if (pEye->IsAggressive())
						wTileImageNo = GetTileImageForEntity(M_EYE_ACTIVE, pMonster->wO, animFrame);
					else {
						if (this->bRequestEvilEyeGaze) // Vision token or puzzle mode can show it
							AddLastLayerEffect(new CEvilEyeGazeEffect(this, pMonster->wX,
								pMonster->wY, pMonster->wO, (UINT)-1));
						
						if (this->puzzleModeOptions.GetShowReverseEvilEyeBeams())
							AddLastLayerEffect(new CEvilEyeGazeEffect(this, pMonster->wX,
								pMonster->wY, nGetReverseO(pMonster->wO), (UINT)-1));
					}
				}
				break;
				case M_AUMTLICH:
				{
					const CAumtlich *pAumtlich = DYN_CAST(const CAumtlich*, const CMonster*, pMonster);
					if (pAumtlich->bFrozen)
						nAddColor = FROZEN_COLOR_INDEX;
				}
				break;
			}

			TileImageBlitParams blit(pMonster->wX, pMonster->wY, wTileImageNo, wXOffset, wYOffset, bMoveInProgress, bDrawRaised);
			blit.nOpacity = opacity;
			blit.nAddColor = nAddColor;
			DrawTileImage(blit, pDestSurface);
		}
		break;
	}
}

//*****************************************************************************
bool CRoomWidget::IsMonsterInvolvedInDeath(CMonster *pMonster) const
//Is this monster either killing the player or a critical character,
//or is a critical character being killed?
{
	ASSERT(pMonster);

	//Did this monster collide with the player?
	UINT wPX, wPY;
	this->pCurrentGame->GetSwordsman(wPX, wPY);
	if (pMonster->wX == wPX && pMonster->wY == wPY)
		return true;
	if (pMonster->IsLongMonster())
	{
		//Did a piece of the monster hit the player?
		const CMonster *pMonsterOnPlayer = this->pRoom->GetOwningMonsterOnSquare(wPX, wPY);
		if (pMonster == pMonsterOnPlayer)
			return true;
	}

	UINT wSX, wSY;
	switch (pMonster->wType)
	{
		case M_CLONE:
		case M_HALPH: case M_HALPH2:
		case M_TEMPORALCLONE:
			//Is critical character being killed (not falling into pit)?
			if (!pMonster->IsAlive() && !bIsPit(this->pRoom->GetOSquare(pMonster->wX, pMonster->wY)))
				return true;
		break;
		case M_CHARACTER:
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (!pCharacter->IsVisible())
				break;

			if (pCharacter->IsMissionCritical()) {
				if (!pCharacter->IsAlive())
					return true;
			} else {
				//Is sworded character killing player or a critical entity?
				if (pCharacter->GetSwordCoords(wSX,wSY))
				{
					if ((wSX == wPX && wSY == wPY) ||
							this->pRoom->IsMonsterOfTypeAt(M_HALPH, wSX, wSY, true, false) ||
							this->pRoom->IsMonsterOfTypeAt(M_HALPH2, wSX, wSY, true, false) ||
							this->pRoom->IsMonsterOfTypeAt(M_CLONE, wSX, wSY, true, false) ||
							this->pRoom->IsMonsterOfTypeAt(M_TEMPORALCLONE, wSX, wSY, true, false))
						return true;
				}
			}
		}
		break;
		default:
			//Is sworded monster killing player or critical character?
			if (pMonster->GetSwordCoords(wSX,wSY))
			{
				if ((wSX == wPX && wSY == wPY) ||
						this->pRoom->IsMonsterOfTypeAt(M_HALPH, wSX, wSY, true, false) ||
						this->pRoom->IsMonsterOfTypeAt(M_HALPH2, wSX, wSY, true, false) ||
						this->pRoom->IsMonsterOfTypeAt(M_CLONE, wSX, wSY, true, false) ||
						this->pRoom->IsMonsterOfTypeAt(M_TEMPORALCLONE, wSX, wSY, true, false)
						)
					return true;
			}
		break;
	}
	return false;
}

//*****************************************************************************
void CRoomWidget::DrawMonsterKillingPlayer(SDL_Surface *pDestSurface)
//Draw one or more monsters killing player or critical character.
{
	ASSERT(pDestSurface);

	//If player is dead, draw monster that made the kill.
	if (this->pCurrentGame->swordsman.IsInRoom())
	{
		DrawMonsterKillingAt(&this->pCurrentGame->swordsman, pDestSurface);
	}

	for (CMonster *pSeek = this->pRoom->pFirstMonster; pSeek != NULL; pSeek = pSeek->pNext)
	{
		//Is critical entity being killed?
		bool bCritical = false;
		switch (pSeek->wType)
		{
			case M_CLONE:
			case M_TEMPORALCLONE:
			case M_HALPH: case M_HALPH2:
				bCritical = true;
			break;
			case M_CHARACTER:
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pSeek);
				bCritical = pCharacter->IsVisible() && pCharacter->IsMissionCritical();
			}
			break;
		}
		//Draw critical character if dead and not falling into pit, plus whatever killed it.
		if (bCritical && !pSeek->IsAlive() && !bIsPit(this->pRoom->GetOSquare(pSeek->wX, pSeek->wY)))
		{
			DrawMonster(pSeek, this->pRoom, pDestSurface, true);
			DrawMonsterKilling(pSeek, pDestSurface);
		}
	}
}

//*****************************************************************************
void CRoomWidget::DrawMonsterKilling(
	CCoord *pCoord,
	SDL_Surface *pDestSurface) //(in) Surface to draw on
{
	const UINT wX = pCoord->wX;
	const UINT wY = pCoord->wY;
	for (int dx=-1; dx<=1; ++dx)
		for (int dy=-1; dy<=1; ++dy)
			if ((dx || dy) && this->pRoom->IsValidColRow(wX+dx,wY+dy))
			{
				CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX+dx,wY+dy);
				if (pMonster && pMonster->HasSwordAt(wX,wY))
				{
					DrawMonster(pMonster, this->pRoom, pDestSurface, true);
					return;
				}
			}
}

//*****************************************************************************
void CRoomWidget::DrawMonsterKillingAt(
//Draw the monster which killed player or critical character.
//Params:
	CCoord *pCoord, //(in) coordinates of dying character
	SDL_Surface *pDestSurface) //(in) Surface to draw on
{
	const CMonster *pMonster = this->pRoom->GetOwningMonsterOnSquare(pCoord->wX, pCoord->wY);
	if (pMonster)
	{
		DrawMonster(pMonster, this->pRoom, pDestSurface, true);
		return;
	}

	DrawMonsterKilling(pCoord, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DrawDoorFiller(SDL_Surface *pDestSurface, const UINT wX, const UINT wY)
//In cases where doors are 2x2 tiles or thicker, draw filler to
//remove the dimple on the edge.
{
	const UINT oTile = this->pRoom->GetOSquare(wX,wY);
	UINT wFillerTI;
	switch (oTile)
	{
		case T_DOOR_Y: wFillerTI = TI_DOOR_YC; break;
		case T_DOOR_M: wFillerTI = TI_DOOR_GC; break;
		case T_DOOR_C: wFillerTI = TI_DOOR_CC; break;
		case T_DOOR_R: wFillerTI = TI_DOOR_RC; break;
		case T_DOOR_B: wFillerTI = TI_DOOR_BC; break;
		default: return; //no filler for this door type
	}

	bool adj[3][3] = {{false}};

	//mark where adjacent door tiles are
	const UINT endX = wX+2, endY = wY+2;
	UINT x, y;
	for (y=wY-1; y!=endY; ++y)
	{
		if (y >= this->pRoom->wRoomRows)
			continue;
		const int dy = y-wY+1;
		for (x=wX-1; x!=endX; ++x)
		{
			if (x >= this->pRoom->wRoomCols)
				continue;
			const int dx = x-wX+1;
			if (dx == 1 && dy == 1)
				continue;
			adj[dx][dy] = (this->pRoom->GetOSquare(x,y) == oTile);
		}
	}

	int nBaseX = this->x + wX * CX_TILE;
	int nBaseY = this->y + wY * CY_TILE;
	const UINT wHalfTileX = CDrodBitmapManager::CX_TILE/2;
	const UINT wHalfTileY = CDrodBitmapManager::CY_TILE/2;

	if (adj[0][0] && adj[0][1] && adj[1][0]) //upper-left corner
	{
		g_pTheDBM->BlitTileImagePart(wFillerTI,
				nBaseX, nBaseY, wHalfTileX, wHalfTileY,
				wHalfTileX, wHalfTileY, pDestSurface);
	}
	if (adj[0][2] && adj[0][1] && adj[1][2]) //lower-left corner
	{
		g_pTheDBM->BlitTileImagePart(wFillerTI,
				nBaseX, nBaseY + wHalfTileY, wHalfTileX, 0,
				wHalfTileX, wHalfTileY, pDestSurface);
	}
	if (adj[2][0] && adj[2][1] && adj[1][0]) //upper-right corner
	{
		g_pTheDBM->BlitTileImagePart(wFillerTI,
				nBaseX + wHalfTileX, nBaseY, 0, wHalfTileY,
				wHalfTileX, wHalfTileY, pDestSurface);
	}
	if (adj[2][2] && adj[2][1] && adj[1][2]) //lower-right corner
	{
		g_pTheDBM->BlitTileImagePart(wFillerTI,
				nBaseX + wHalfTileX, nBaseY + wHalfTileY, 0, 0,
				wHalfTileX, wHalfTileY, pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::DrawTileImage(
//Blits a tile graphic to a specified room position.
//Dirties tiles covered by this blit.
//
//Params:
	const TileImageBlitParams& blit,
	SDL_Surface *pDestSurface)    //(in)   Surface to draw to.
{
	DrawTileImageWithoutLight(blit, pDestSurface);

	//Set to proper light level.
	DrawTileLight(blit, pDestSurface);
}


//*****************************************************************************
void CRoomWidget::DrawTileImageWithoutLight(
//Blits a tile graphic to a specified room position.
//Dirties tiles covered by this blit.
//
//Params:
	const TileImageBlitParams& blit,
	SDL_Surface *pDestSurface)    //(in)   Surface to draw to.
{
	if (blit.wTileImageNo == TI_TEMPTY)
		return; //Wasteful to make this call for empty blit.

	const UINT wCol = blit.wCol;
	const UINT wRow = blit.wRow;

	bool bClipped = blit.bClipped;
	ASSERT(IS_COLROW_IN_DISP(wCol, wRow) || bClipped);

	//Determine pixel positions.
	const int nRoomPixelX = (CX_TILE * wCol) + blit.wXOffset;
	int nRoomPixelY = (CY_TILE * wRow) + blit.wYOffset;
	if (blit.bDrawRaised)
	{
		nRoomPixelY -= CY_RAISED;
		if (wRow == 0)
			bClipped = true;
	}
	int nPixelX = this->x + nRoomPixelX;
	int nPixelY = this->y + nRoomPixelY;

	SDL_Rect BlitRect = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	if (bClipped && !ClipTileArea(nPixelX, nPixelY, BlitRect))
		return;

	ASSERT(BlitRect.w <= CX_TILE);
	ASSERT(BlitRect.h <= CY_TILE);

	if (!TileImageBlitParams::CropRectToTileDisplayArea(BlitRect))
		return;

	nPixelX += BlitRect.x;
	nPixelY += BlitRect.y;
	ASSERT(nPixelX >= this->x); //after cropping, blit should be placed inside room
	ASSERT(nPixelY >= this->y);
	ASSERT(nPixelX + BlitRect.w <= this->x + int(this->w));
	ASSERT(nPixelY + BlitRect.h <= this->y + int(this->h));

	//Draw sprite.
	g_pTheDBM->BlitTileImagePart(blit.wTileImageNo,
			nPixelX, nPixelY,
			BlitRect.x, BlitRect.y, BlitRect.w, BlitRect.h,
			pDestSurface, true, blit.nOpacity);

	//Add optional color filter.
	if (blit.nAddColor >= 0)
		g_pTheBM->LightenRectWithTileMask(pDestSurface, nPixelX, nPixelY,
				BlitRect.w, BlitRect.h,
				1.0f+lightMap[0][blit.nAddColor], 1.0f+lightMap[1][blit.nAddColor], 1.0f+lightMap[2][blit.nAddColor],
				blit.wTileImageNo, BlitRect.x, BlitRect.y);

	if (blit.nCustomColor != -1) {
		float fR, fG, fB;
		TranslateMonsterColor(blit.nCustomColor, fR, fG, fB);

		g_pTheBM->LightenRectWithTileMask(pDestSurface, nPixelX, nPixelY,
			BlitRect.w, BlitRect.h,
			fR, fG, fB,
			blit.wTileImageNo, BlitRect.x, BlitRect.y);
	}

	if (!blit.bDirtyTiles)
	{
		//The 'monster' flag indicates that something was drawn here, and that
		//at the latest the tile should be repainted next turn.
		if (IS_COLROW_IN_DISP(wCol, wRow))
			this->pTileImages[this->pRoom->ARRAYINDEX(wCol, wRow)].monster = 1;
	}
	else {
		//Dirty tiles covered by blit.
		DirtyTilesForSpriteAt(nRoomPixelX + BlitRect.x, nRoomPixelY + BlitRect.y, BlitRect.w, BlitRect.h);
	}
}

//*****************************************************************************
void CRoomWidget::DrawTileLight(
//Blits tile lighting on specific position
//
//Params:
	const TileImageBlitParams& blit,
	SDL_Surface *pDestSurface)    //(in)   Surface to draw to.
{
	if (blit.wTileImageNo == TI_TEMPTY)
		return; //Wasteful to make this call for empty blit.

	if (!IsLightingRendered())
		return;

	bool bClipped = blit.bClipped;
	ASSERT(IS_COLROW_IN_DISP(blit.wCol, blit.wRow) || bClipped);

	//Determine pixel positions.
	const int nRoomPixelX = (CX_TILE * blit.wCol) + blit.wXOffset;
	int nRoomPixelY = (CY_TILE * blit.wRow) + blit.wYOffset;
	if (blit.bDrawRaised)
	{
		nRoomPixelY -= CY_RAISED;
		if (blit.wRow == 0)
			bClipped = true;
	}
	int nPixelX = this->x + nRoomPixelX;
	int nPixelY = this->y + nRoomPixelY;

	SDL_Rect BlitRect = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	if (bClipped && !ClipTileArea(nPixelX, nPixelY, BlitRect))
		return;

	ASSERT(BlitRect.w <= CX_TILE);
	ASSERT(BlitRect.h <= CY_TILE);

	if (!TileImageBlitParams::CropRectToTileDisplayArea(BlitRect))
		return;

	nPixelX += BlitRect.x;
	nPixelY += BlitRect.y;
	ASSERT(nPixelX >= this->x); //after cropping, blit should be placed inside room
	ASSERT(nPixelY >= this->y);
	ASSERT(nPixelX + BlitRect.w <= this->x + int(this->w));
	ASSERT(nPixelY + BlitRect.h <= this->y + int(this->h));

	//Set to proper light level.
	const bool bMoving = blit.wXOffset || blit.wYOffset;

	int nCol = int(blit.wCol);
	int nRow = int(blit.wRow);
	if (!this->pRoom->IsValidColRow(nCol, nRow)) {
		if (!bMoving || !bClipped)
			return;

		//When sprite is moving into/out of room,
		//use the lighting on the tile being moved into/out of for illumination
		nCol += sgn(blit.wXOffset);
		nRow += sgn(blit.wYOffset);
		if (!this->pRoom->IsValidColRow(nCol, nRow)) //can happen when moving across a room corner
			this->pRoom->ClampCoordsToRoom(nCol, nRow);
	}

	SDL_Rect *pCropRect = TileImageBlitParams::crop_display || bClipped ? &BlitRect : NULL;

	//Add cast shadows to sprite.
	if (blit.bCastShadowsOnTop && !blit.bDrawRaised)
	{
		if (!bMoving)
		{
			const vector<UINT> &shadows = this->pTileImages[this->pRoom->ARRAYINDEX(nCol, nRow)].shadowMasks;
			if (!shadows.empty())
				g_pTheBM->BlitTileShadowsWithTileMask(&(shadows[0]), shadows.size(),
					blit.wTileImageNo, nPixelX, nPixelY, pDestSurface, pCropRect);
		} else if (!pCropRect) { //routine doesn't support cropping
			BlitTileShadowsOnMovingSprite(blit, pDestSurface);
		}
	}

	//Apply lighting.
	if (bMoving && !pCropRect)
	{
		AddLightOffset(pDestSurface, blit);
	} else {
		//Add dark to sprite.
		const float fDark = GetOverheadDarknessAt(nCol, nRow, blit.appliedDarkness);

		//Add light to sprite.
		if (this->pRoom->tileLights.Exists(nCol, nRow) ||
				this->lightedRoomTiles.has(nCol, nRow) || this->lightedPlayerTiles.has(nCol, nRow))
		{
			LIGHTTYPE *psL = this->lightMaps.psDisplayedLight + (nRow * this->pRoom->wRoomCols + nCol) * wLightValuesPerTile;
			if (bMoving) {
				AddLight(pDestSurface, nPixelX - BlitRect.x, nPixelY - BlitRect.y, psL, fDark,
						blit.wTileImageNo, blit.nOpacity, pCropRect); //faster lighting when moving
			} else {
				AddLightInterp(pDestSurface, nCol, nRow, psL, fDark, blit.wTileImageNo,
						blit.nOpacity, blit.bDrawRaised ? -int(CY_RAISED) : 0, pCropRect);
			}
		}
	}
}

//*****************************************************************************
void CRoomWidget::BlitTileShadowsOnMovingSprite(
	const TileImageBlitParams& blit,
	SDL_Surface *pDestSurface)
{
	int xOffset = blit.wXOffset, yOffset = blit.wYOffset;
	ASSERT(xOffset || yOffset); //sprite is moving

	if (abs(xOffset) > CX_TILE || abs(yOffset) > CY_TILE) //doesn't support multi-tile movement
		return;

	const UINT wCol = blit.wCol - (xOffset < 0 ? 1 : 0);
	const UINT wRow = blit.wRow - (yOffset < 0 ? 1 : 0);

	ASSERT(IS_COLROW_IN_DISP(wCol, wRow));
	ASSERT(!blit.bDrawRaised);

	//Get positive offset values.
	while (xOffset < 0)
		xOffset += CX_TILE;
	while (yOffset < 0)
		yOffset += CY_TILE;

	//Determine pixel positions.
	const int nPixelX = this->x + (CX_TILE * wCol);
	const int nPixelY = this->y + (CY_TILE * wRow);

	const vector<UINT> *pShadows;

	//(xL,yT)
	pShadows = &(this->pTileImages[this->pRoom->ARRAYINDEX(wCol,wRow)].shadowMasks);
	if (!pShadows->empty()) {
		SDL_Rect shadowCrop = MAKE_SDL_RECT(xOffset, yOffset, CX_TILE - xOffset, CY_TILE - yOffset);

		g_pTheBM->BlitTileShadowsWithTileMask(&((*pShadows)[0]), pShadows->size(),
			blit.wTileImageNo, nPixelX + shadowCrop.x, nPixelY + shadowCrop.y,
			pDestSurface, &shadowCrop, 0, 0);
	}

	const UINT nextCol = wCol+1, nextRow = wRow+1;
	if (blit.wYOffset && nextRow < this->pRoom->wRoomRows) {
		//(xL, yB)
		pShadows = &(this->pTileImages[this->pRoom->ARRAYINDEX(wCol,nextRow)].shadowMasks);
		if (!pShadows->empty()) {
			SDL_Rect shadowCrop = MAKE_SDL_RECT(xOffset, 0, CX_TILE - xOffset, yOffset);

			g_pTheBM->BlitTileShadowsWithTileMask(&((*pShadows)[0]), pShadows->size(),
				blit.wTileImageNo, nPixelX + shadowCrop.x, nPixelY + CY_TILE + shadowCrop.y,
				pDestSurface, &shadowCrop, 0, CY_TILE - shadowCrop.h);
		}
	}

	if (blit.wXOffset && nextCol < this->pRoom->wRoomCols) {
		//(xR, yT)
		pShadows = &(this->pTileImages[this->pRoom->ARRAYINDEX(nextCol,wRow)].shadowMasks);
		if (!pShadows->empty()) {
			SDL_Rect shadowCrop = MAKE_SDL_RECT(0, yOffset, xOffset, CY_TILE - yOffset);

			g_pTheBM->BlitTileShadowsWithTileMask(&((*pShadows)[0]), pShadows->size(),
				blit.wTileImageNo, nPixelX + CX_TILE + shadowCrop.x, nPixelY + shadowCrop.y,
				pDestSurface, &shadowCrop, CX_TILE - shadowCrop.w, 0);
		}

		if (blit.wYOffset && nextRow < this->pRoom->wRoomRows) {
			//(xR, yB)
			pShadows = &(this->pTileImages[this->pRoom->ARRAYINDEX(nextCol,nextRow)].shadowMasks);
			if (!pShadows->empty()) {
				SDL_Rect shadowCrop = MAKE_SDL_RECT(0, 0, xOffset, yOffset);

				g_pTheBM->BlitTileShadowsWithTileMask(&((*pShadows)[0]), pShadows->size(),
					blit.wTileImageNo, nPixelX + CX_TILE + shadowCrop.x, nPixelY + CY_TILE + shadowCrop.y,
					pDestSurface, &shadowCrop, CX_TILE - shadowCrop.w, CY_TILE - shadowCrop.h);
			}
		}
	}
}

//*****************************************************************************
bool CRoomWidget::ClipTileArea(
	int nPixelX, int nPixelY,
	SDL_Rect& BlitRect) //(in/out)
const
{
	//Calculate what part of tile is showing.
	SDL_Rect ClipRect;
	GetRect(ClipRect);
	if (nPixelX < ClipRect.x)
	{
		const int origX = BlitRect.x;
		BlitRect.x = ClipRect.x - nPixelX;
		ASSERT(BlitRect.x > 0);
		const int delta = BlitRect.x - origX;
		if (delta >= BlitRect.w)
			return false; //completely outside rect
		BlitRect.w -= delta; //trim width by amount x is brought in
	}
	else if (nPixelX + (int)BlitRect.w >= ClipRect.x + (int)ClipRect.w)
	{
		if (nPixelX >= ClipRect.x + (int)ClipRect.w)
			return false; //completely outside rect
		BlitRect.w = (ClipRect.x + ClipRect.w) - nPixelX;
	}
	if (nPixelY < ClipRect.y)
	{
		const int origY = BlitRect.y;
		BlitRect.y = ClipRect.y - nPixelY;
		ASSERT(BlitRect.y > 0);
		const int delta = BlitRect.y - origY;
		if (delta >= BlitRect.h)
			return false; //completely outside rect
		BlitRect.h -= delta; //trim height by amount y is brought in
	}
	else if (nPixelY + (int)BlitRect.h >= ClipRect.y + (int)ClipRect.h)
	{
		if (nPixelY >= ClipRect.y + (int)ClipRect.h)
			return false; //completely outside rect
		BlitRect.h = (ClipRect.y + ClipRect.h) - nPixelY;
	}
	return true;
}

//*****************************************************************************
void CRoomWidget::DirtyTilesForSpriteAt(UINT pixel_x, UINT pixel_y, UINT w, UINT h)
{
	//There are up to four tiles dirtied by a tile blit at this pixel location.
	UINT wMinX = pixel_x/CX_TILE;
	UINT wMinY = pixel_y/CY_TILE;
	const UINT wMaxX = (pixel_x+w-1)/CX_TILE;
	const UINT wMaxY = (pixel_y+h-1)/CY_TILE;
	if (wMinX > wMaxX) wMinX = wMaxX;   //bounds checking
	if (wMinY > wMaxY) wMinY = wMaxY;

	for (UINT wY = wMinY; wY <= wMaxY; ++wY)
		for (UINT wX = wMinX; wX <= wMaxX; ++wX)
			if (IS_COLROW_IN_DISP(wX, wY))
				this->pTileImages[this->pRoom->ARRAYINDEX(wX,wY)].dirty = 1;
}

//*****************************************************************************
void CRoomWidget::DrawBoltInRoom(
//Draws a lightning bolt between the given coords.
//
//Params:
	const int xS, const int yS, const int xC, const int yC)
{
	static SDL_Rect RoomRect;
	SDL_Surface *pDestSurface = GetDestSurface();
	GetRect(RoomRect);
	SDL_SetClipRect(pDestSurface, &RoomRect);
	vector<SDL_Rect> dirtyRects;

	DrawBolt(xS, yS, xC, yC, CDrodBitmapManager::DISPLAY_COLS,
			this->images[BOLTS_SURFACE], pDestSurface, dirtyRects);
	this->pLastLayerEffects->DirtyTilesForRects(dirtyRects);

	SDL_SetClipRect(pDestSurface, NULL);
}

//*****************************************************************************
void CRoomWidget::DrawInvisibilityRange(
//Show effective range of invisibility/decoy effect by highlighting area around
//target.
//
//Params:
	int nX, int nY,   //(in)   Center of effect
	SDL_Surface *pDestSurface,    //(in)   Surface to draw to.
	CCoordIndex *pCoordIndex,		//(in/out)  which squares are drawn to [default=NULL]
		//if specified, only drawn to unmarked squares
	const int nRange) //[default=DEFAULT_SMELL_RANGE]
{
	ASSERT(nRange >= 0);
	ASSERT(this->pRoom->IsValidColRow(nX,nY));
	const int numTiles = nRange*2 + 1;
	const int x1 = nX - nRange;
	const int y1 = nY - nRange;
	const int x2 = nX + nRange;
	const int y2 = nY + nRange;
	int wX, wY;

	if (!pDestSurface)
	{
		//Show as lasting effect, rather than just painting this frame.
		static const SURFACECOLOR DecoyColor = {224, 160, 0};
		for (wY = y1; wY <= y2; ++wY)
			for (wX = x1; wX <= x2; ++wX)
			{
				if (!IS_COLROW_IN_DISP(wX,wY)) continue;
				if (pCoordIndex)
				{
					if (pCoordIndex->Exists(wX,wY)) continue;
					else pCoordIndex->Add(wX,wY);
				}
				AddShadeEffect(wX, wY, DecoyColor);
			}
	} else {
		static const float fShadingFactor = 0.80f;
		if (this->bAllDirty && !pCoordIndex)
			DarkenRect(pDestSurface, fShadingFactor, x1, y1, numTiles, numTiles);
		else {
			//Darken only dirty tiles.
			for (wY = y1; wY <= y2; ++wY)
			{
				UINT index = this->pRoom->ARRAYINDEX(x1,wY);
				for (wX = x1; wX <= x2; ++wX, ++index)
				{
					if (!IS_COLROW_IN_DISP(wX,wY)) continue;
					const TileImages& ti = this->pTileImages[index];
					if (this->bAllDirty || ti.dirty || ti.damaged)
					{
						if (pCoordIndex)
						{
							if (pCoordIndex->Exists(wX,wY)) continue;
							else pCoordIndex->Add(wX,wY);
						}

						g_pTheBM->DarkenTile(this->x + wX * CX_TILE,
								this->y + wY * CY_TILE, fShadingFactor, pDestSurface);
					}
				}
			}
		}
		DirtyTileRect(x1-1,y1-1,x2+1,y2+1); //one tile buffer in case swordsman moved
	}
}

//*****************************************************************************
void CRoomWidget::DrawOverheadLayer(SDL_Surface *pDestSurface)
{
	ASSERT(pDestSurface);

	const CCoordIndex& tiles = this->pRoom->overheadTiles;
	if (tiles.empty())
		return;

	SDL_Surface *pTileTexture = g_pTheDBM->pTextures[OVERHEAD_IMAGE];
	if (!pTileTexture)
		return;

	const UINT wWidth = pTileTexture->w;
	const UINT wHeight = pTileTexture->h;
	int nI;

	UINT index=0;
	for (UINT wY=0; wY < this->pRoom->wRoomRows; ++wY) {
		for (UINT wX=0; wX < this->pRoom->wRoomCols; ++wX, ++index) {
			if (tiles.Exists(wX, wY)) {
				TileImages& ti = this->pTileImages[index];
				if (this->bAllDirty || ti.dirty || ti.damaged)
				{
					ti.dirty = 1;

					//Calculate coords for custom texture from indicated origin.
					const int pixelX = this->x + wX * CX_TILE;
					const int pixelY = this->y + wY * CY_TILE;

					SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
					SDL_Rect dest = MAKE_SDL_RECT(pixelX, pixelY, CX_TILE, CY_TILE);

					nI = (int(wX)-int(this->pRoom->wOverheadImageStartX)) * CX_TILE;
					while (nI < 0)
						nI += wWidth;
					src.x = nI % wWidth;

					nI = (int(wY)-int(this->pRoom->wOverheadImageStartY)) * CY_TILE;
					while (nI < 0)
						nI += wHeight;
					src.y = nI % wHeight;

					SDL_BlitSurface(pTileTexture, &src, pDestSurface, &dest);
				}
			}
		}
	}
}

//*****************************************************************************
void CRoomWidget::DrawPlayer(
//Draws player along with his sword (if applicable) to game screen.
//
//Params:
	const CSwordsman &swordsman,  //(in) swordsman data
	SDL_Surface *pDestSurface)    //(in) Surface to draw to.
{
	ASSERT(IS_COLROW_IN_DISP(swordsman.wX, swordsman.wY));
	ASSERT(this->pRoom);

	static Uint8 nOpacity = 255;
	static bool bFadeIn = false;

	UINT wO, wFrame, wSManTI, wSwordTI;
	if (!GetPlayerDisplayTiles(swordsman, wO, wFrame, wSManTI, wSwordTI))
		return;

	const bool bPlayerIsInvisible = !swordsman.IsVisible() && !swordsman.bIsDying;
	if (bPlayerIsInvisible)
		DrawInvisibilityRange(swordsman.wX, swordsman.wY, pDestSurface);

	if (swordsman.bIsDying)
		nOpacity = 255;
	else if (swordsman.bIsHasted)
	{
		//Speed potion effect: fast flashing.
		if (nOpacity < 20)
			bFadeIn = true;
		else if (nOpacity > 235)
			bFadeIn = false;
		if (bFadeIn)
			nOpacity += 15;
		else
			nOpacity -= 15;
	}
	else if (!swordsman.IsVisible())
	{
		//Invisible.  Fade in and out slowly.
		if (nOpacity < 50)   //don't make too faint
			bFadeIn = true;
		else if (nOpacity > 160)   //don't make too dark
			bFadeIn = false;
		if (bFadeIn)
			nOpacity += 2;
		else
			nOpacity -= 8;
	}
	else nOpacity = 255;

	//Draw swordsman raised above floor?
	const UINT wOSquare = this->pRoom->GetOSquare(swordsman.wX, swordsman.wY);
	const bool bDrawRaised = DrawRaised(wOSquare) && wOSquare != T_WALL_M && wOSquare != T_WALL_WIN;

	//Calculate animation offset.
	UINT wXOffset = 0, wYOffset = 0;
	AddJitterOffset(swordsman.wX, swordsman.wY, wXOffset, wYOffset);

	//Get player sword position.
	//Also check for jitter bounding.
	//Call this before the offset positioning code below so
	//jitter will be kept inside of the room area so light mixing works properly.
	UINT wSX, wSY;
	const bool bHasWeapon = swordsman.HasWeapon();
	if (bHasWeapon)
	{
		wSX = swordsman.wSwordX;
		wSY = swordsman.wSwordY;
		JitterBoundsCheck(wSX, wSY, wXOffset, wYOffset);
	}

	//Get movement offset.
	int nSgnX = 0, nSgnY = 0;
	if (this->dwMovementStepsLeft)
	{
		wXOffset += (swordsman.wPrevX - swordsman.wX) * this->dwMovementStepsLeft;
		wYOffset += (swordsman.wPrevY - swordsman.wY) * this->dwMovementStepsLeft;
		nSgnX = swordsman.wPrevX == swordsman.wX ? 0 : swordsman.wPrevX > swordsman.wX ?
			1 : -1;
		nSgnY = swordsman.wPrevY == swordsman.wY ? 0 : swordsman.wPrevY > swordsman.wY ?
			1 : -1;
	}

	//Blit the player.
	{
		const int nAddColor = swordsman.bFrozen ? FROZEN_COLOR_INDEX : NO_COLOR_INDEX;
		TileImageBlitParams blit(swordsman.wX, swordsman.wY, wSManTI, wXOffset, wYOffset, true, bDrawRaised);
		blit.nOpacity = nOpacity;
		blit.nAddColor = nAddColor;
		DrawTileImage(blit, pDestSurface);
	}

	//Blit the weapon.
	if (!bHasWeapon)
		return;

	//Special case: don't draw sword when going up/down stairs and sword is occluded.
	if (bIsStairs(wOSquare) &&
			wO != (wOSquare == T_STAIRS ? N : S) &&
			(!this->pRoom->IsValidColRow(wSX, wSY) ||
			!bIsStairs(this->pRoom->GetOSquare(wSX, wSY))))
		return;	//sword should be hidden

	//If sword is not fully in display, draw it clipped.
	//This is needed when raised at top edge or stepping onto room edge.
	const bool bClipped = !IS_COLROW_IN_DISP(wSX, wSY) ||
			!IS_COLROW_IN_DISP(wSX + nSgnX, wSY + nSgnY);

	TileImageBlitParams blit(wSX, wSY, wSwordTI, wXOffset, wYOffset, true, bDrawRaised);
	blit.nOpacity = nOpacity;
	blit.bClipped = bClipped;
	DrawTileImage(blit, pDestSurface);
}

//*****************************************************************************
bool CRoomWidget::GetPlayerDisplayTiles(
	const CSwordsman &swordsman,
	UINT& wO, UINT& wFrame, UINT& wSManTI, UINT& wSwordTI)
const
{
	//Make sure display orientation is correct for role appearance.
	switch (int(swordsman.wAppearance))
	{
		case M_NONE: return false; //Don't show anything if player is not being shown.
		case M_BRAIN: case M_SKIPPERNEST: wO = NO_ORIENTATION; break;
		default: wO = swordsman.wO; break;
	}
	ASSERT(IsValidOrientation(wO));

	//Get player animation frame.
	const bool bWading = (this->pCurrentGame && this->pCurrentGame->IsPlayerWading() && bIsWadingMonsterType(swordsman.wAppearance)) ||
		(swordsman.bIsDying && bIsWater(this->pRoom->GetOSquare(swordsman.wX, swordsman.wY)));
	if (swordsman.HasWeapon() || !bEntityHasSword(swordsman.wAppearance)) {
		if (bWading) {
			wFrame = (UINT)SWORD_WADING_FRAME;
		} else {
			wFrame = this->pTileImages[this->pRoom->ARRAYINDEX(swordsman.wX, swordsman.wY)].animFrame;
			if (!IsMonsterTypeAnimated(swordsman.wAppearance))
				wFrame = wFrame % ANIMATION_FRAMES;
		}
	} else {
		//If player is a swordless sword-wielding character, then try to show the swordless frame.
		wFrame = bWading ? WADING_FRAME : SWORDLESS_FRAME;
	}

	wSManTI = GetCustomEntityTile(swordsman.wIdentity, wO, wFrame);

	//Get optional custom sword tile.  It overrides other sword display variants below.
	wSwordTI = TI_UNSPECIFIED;
	if (this->pCurrentGame)
	{
		ASSERT(this->pCurrentGame->pHold);
		HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacter(
				swordsman.wIdentity);
		if (pCustomChar)
			wSwordTI = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles,
					GetCustomTileIndex(wO), SWORD_FRAME);
	}

	const bool bPlayerIsInvisible = !swordsman.IsVisible() && !swordsman.bIsDying;
	if (bPlayerIsInvisible)
	{
		const bool bBeethroLike = bIsBeethroDouble(swordsman.wAppearance) &&
				swordsman.wAppearance != M_GUNTHRO;
		if (!this->pRoom->SomeMonsterCanSmellSwordsman() && bBeethroLike)
		{
			//If invisible player is not sensed by any monsters, and is
			//in a role that looks like Beethro, then show "hollow" version.
			//(Custom tiles specified above will override this behavior.)
			if (wSManTI == TI_UNSPECIFIED)
				wSManTI = GetTileImageForEntity(UINT(M_DECOY), wO, wFrame);
			//Have a hollow sword variant.
			if (wSwordTI == TI_UNSPECIFIED && swordsman.GetActiveWeapon() == WT_Sword)
				wSwordTI = GetSwordTile(M_DECOY, wO);
		}
	}

	//If no custom or situation-specific player/sword tile is provided, use the stock/default.
	if (wSManTI == TI_UNSPECIFIED) {
		wSManTI = GetStockEntityTile(swordsman.wAppearance == static_cast<UINT>(-1) ?
			static_cast<UINT>(CHARACTER_FIRST) : swordsman.wAppearance, wO, wFrame);
	}
	if (wSwordTI == TI_UNSPECIFIED && bEntityHasSword(swordsman.wAppearance))
		wSwordTI = GetSwordTile(swordsman.wAppearance, wO, swordsman.GetActiveWeapon());

	return true;
}

//*****************************************************************************
void CRoomWidget::DrawCharacter(
//Draws character if visible, according to set appearance.
//
//Params:
	const CCharacter *pCharacter, //(in)   Pointer to CCharacter monster.
	bool bDrawRaised,    //(in)   Draw Character raised above floor?
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	if (!pCharacter->Display()) return;

	Uint8 nOpacity = 255;
	int nAddColor = NO_COLOR_INDEX;
	UINT wIdentity = pCharacter->GetIdentity();
	if (wIdentity == M_CLONE || wIdentity == M_TEMPORALCLONE)
	{
		//Resolve identity of clone to actual role, and give clone color highlighting.
		wIdentity = pCharacter->GetResolvedIdentity();
		nAddColor = CLONE_COLOR_INDEX; //light green-blue to clones

		//Clones are invisible when the player is invisible.
		if (this->pCurrentGame)
		{
			const CSwordsman& player = this->pCurrentGame->swordsman;
			bool bInvisible = player.bIsInvisible;
			if (!bInvisible)
			{
				const UINT wOSquare = this->pRoom->GetOSquare(pCharacter->wX, pCharacter->wY);
				if ((wOSquare == T_SHALLOW_WATER) && (player.GetWaterTraversalState(wIdentity) == WTrv_CanHide))
					bInvisible = true;
			}
			if (bInvisible)
				nOpacity = 128;
		}
	}

	ASSERT(wIdentity < MONSTER_COUNT ||
			(wIdentity >= CHARACTER_FIRST && wIdentity < CHARACTER_TYPES) ||
			wIdentity == static_cast<UINT>(-1));
	const UINT wO = wIdentity == M_BRAIN || wIdentity == M_SKIPPERNEST ?
			NO_ORIENTATION : pCharacter->wO;

	//If a sword-wielding character is swordless, try to get its swordless frame.
	//However, if that frame is not defined or otherwise, then show the current animation frame.
	UINT wSX, wSY;
	const bool bHasSword = pCharacter->GetSwordCoords(wSX, wSY);
	UINT wFrame;
	if (bHasSword || !bEntityHasSword(wIdentity)) {
		wFrame = this->pTileImages[this->pRoom->ARRAYINDEX(pCharacter->wX, pCharacter->wY)].animFrame;
		if (!IsMonsterTypeAnimated(wIdentity))
			wFrame = wFrame % ANIMATION_FRAMES;
	} else {
		wFrame = (UINT)SWORDLESS_FRAME;
	}
	if (pCharacter->IsWading()) {
		wFrame = bHasSword ? SWORD_WADING_FRAME : WADING_FRAME;
	}
	UINT wTileImageNo = GetEntityTile(wIdentity, pCharacter->wLogicalIdentity, wO, wFrame);

	//Calculate animation offset.
	UINT wXOffset = 0, wYOffset = 0;
	if (this->dwMovementStepsLeft)
	{
		wXOffset = (pCharacter->wPrevX - pCharacter->wX) * this->dwMovementStepsLeft;
		wYOffset = (pCharacter->wPrevY - pCharacter->wY) * this->dwMovementStepsLeft;
	}

	if (pCharacter->DisplayGhostImage()) {
		bDrawRaised = false;
	} else {
		AddJitterOffset(pCharacter->wX, pCharacter->wY, wXOffset, wYOffset);
	}

	//Draw character.
	TileImageBlitParams blit(pCharacter->wX, pCharacter->wY, wTileImageNo, wXOffset, wYOffset, bMoveInProgress, bDrawRaised);
	blit.nOpacity = nOpacity;
	blit.nCustomColor = pCharacter->nColor;
	blit.nAddColor = nAddColor;
	DrawTileImage(blit, pDestSurface);

	//Draw character with sword.
	if (bHasSword) {
		blit.wCol = wSX;
		blit.wRow = wSY;
		blit.nAddColor = NO_COLOR_INDEX;
		DrawSwordFor(pCharacter, wIdentity, blit, pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::DrawCitizen(
//Draws character if visible, according to set appearance.
//
//Params:
	const CCitizen *pCitizen,  //(in)   Pointer to CCitizen monster.
	const bool bDrawRaised,    //(in)   Draw raised above floor?
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	const bool bWading = pCitizen->IsWading();
	UINT wFrame;
	if (bWading) {
		wFrame = (UINT)WADING_FRAME;
	} else {
		ASSERT(!IsMonsterTypeAnimated(M_CITIZEN));
		wFrame = this->pTileImages[this->pRoom->ARRAYINDEX(pCitizen->wX, pCitizen->wY)].animFrame % ANIMATION_FRAMES;
	}

	const UINT wTileImageNo = GetTileImageForEntity(M_CITIZEN, pCitizen->wO, wFrame);
	//Calculate animation offset.
	UINT wXOffset = 0, wYOffset = 0;
	if (this->dwMovementStepsLeft)
	{
		wXOffset = (pCitizen->wPrevX - pCitizen->wX) * this->dwMovementStepsLeft;
		wYOffset = (pCitizen->wPrevY - pCitizen->wY) * this->dwMovementStepsLeft;
	}

	TileImageBlitParams blit(pCitizen->wX, pCitizen->wY, wTileImageNo, wXOffset, wYOffset, bMoveInProgress, bDrawRaised);
	blit.nAddColor = pCitizen->StationType();
	DrawTileImage(blit, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DrawArmedMonster(
//Draws player double along with his sword to room.
//
//Params:
	const CArmedMonster *pArmedMonster, //(in)   Pointer to monster.
	const bool bDrawRaised,    //(in)   Draw double raised above floor?
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress,
	const Uint8 nOpacity)      //(in)   Opacity of double [default=255]
{
	ASSERT(pArmedMonster);
	ASSERT(IsValidOrientation(pArmedMonster->wO));
	ASSERT(IS_COLROW_IN_DISP(pArmedMonster->wX, pArmedMonster->wY));

	//Calculate animation offset.
	UINT wXOffset = 0, wYOffset = 0;
	if (this->dwMovementStepsLeft)
	{
		wXOffset = (pArmedMonster->wPrevX - pArmedMonster->wX) * this->dwMovementStepsLeft;
		wYOffset = (pArmedMonster->wPrevY - pArmedMonster->wY) * this->dwMovementStepsLeft;
	}
	AddJitterOffset(pArmedMonster->wX, pArmedMonster->wY, wXOffset, wYOffset);

	//Blit the double.
	const bool bHasSword = pArmedMonster->HasSword();
	const bool bWading = pArmedMonster->IsWading() && bIsWadingMonsterType(pArmedMonster->wType);
	UINT wFrame = 0;
	if (bWading) {
		wFrame = bHasSword ? SWORD_WADING_FRAME : WADING_FRAME;
	} else if (!bHasSword) {
		wFrame = SWORDLESS_FRAME;
	}

	const UINT logicalIdentity = pArmedMonster->GetIdentity();
	const UINT apparentIdentity = getApparentIdentity(logicalIdentity);
	const UINT wTI = GetEntityTile(apparentIdentity, logicalIdentity, pArmedMonster->wO, wFrame);
	const UINT wSwordTI = bHasSword ? GetSwordTileFor(pArmedMonster, pArmedMonster->wO, logicalIdentity) : TI_UNSPECIFIED;

	TileImageBlitParams blit(pArmedMonster->wX, pArmedMonster->wY, wTI, wXOffset, wYOffset, bMoveInProgress, bDrawRaised);
	blit.nOpacity = nOpacity;

	switch (pArmedMonster->wType)
	{
		case M_CLONE:
			blit.nAddColor = CLONE_COLOR_INDEX; //light green-blue to clones
			//Clone may also look like a custom player tileset.
			if (this->pCurrentGame)
			{
				CSwordsman& player = this->pCurrentGame->swordsman;
				const UINT wSManTI = GetCustomEntityTile(player.wIdentity, pArmedMonster->wO, wFrame);
				if (wSManTI != TI_UNSPECIFIED)
					blit.wTileImageNo = wSManTI;

				//Invisible clones are drawn with transparency
				if (pArmedMonster->IsHiding())
					blit.nOpacity = 128;
			}
			if (pArmedMonster->bFrozen)
				blit.nAddColor = FROZEN_COLOR_INDEX;
		break;
		case M_TEMPORALCLONE:
			if (this->pCurrentGame)
			{
				const CTemporalClone *pTC = DYN_CAST(const CTemporalClone*, const CArmedMonster*, pArmedMonster);
				const UINT wSManTI = GetCustomEntityTile(pTC->wIdentity, pTC->wO, wFrame);
				if (wSManTI != TI_UNSPECIFIED)
					blit.wTileImageNo = wSManTI;

				if (pTC->IsHiding())
					blit.nOpacity = 128;

				AddTemporalCloneNextMoveEffect(pTC, wFrame);
			}
			if (pArmedMonster->bFrozen)
				blit.nAddColor = FROZEN_COLOR_INDEX;
		break;
		case M_STALWART: case M_STALWART2:
		case M_MIMIC:
			if (pArmedMonster->bFrozen)
				blit.nAddColor = FROZEN_COLOR_INDEX;
		break;
	}

	//Draw temporal clone in two vertical slices.  Show a gap between them.
	const bool bTemporalCloneEffect = pArmedMonster->wType == M_TEMPORALCLONE && IsTemporalCloneAnimated();
	if (bTemporalCloneEffect)
	{
		blit.bDirtyTiles = true;

		//Draw top half.
		if (this->temporalCloneEffectHeight > 0) {
			TileImageBlitParams::setDisplayArea(0, 0, CX_TILE, this->temporalCloneEffectHeight);
			DrawTileImage(blit, pDestSurface);
			if (bHasSword) {
				TileImageBlitParams weaponBlit(blit);
				weaponBlit.wCol = pArmedMonster->GetWeaponX();
				weaponBlit.wRow = pArmedMonster->GetWeaponY();
				weaponBlit.nAddColor = -1;
				weaponBlit.wTileImageNo = wSwordTI;
				DrawSwordFor(pArmedMonster, weaponBlit, pDestSurface);
			}
		}

		//Prepare to draw bottom half.
		static const int GAP_PIXELS = 2;
		const int y_bottom = this->temporalCloneEffectHeight + 1 + RAND(GAP_PIXELS);
		if (y_bottom >= CY_TILE)
		{
			//nothing showing for bottom half
			TileImageBlitParams::resetDisplayArea();
			return;
		}

		//Set area to display for bottom half.
		TileImageBlitParams::setDisplayArea(0, y_bottom, CX_TILE, CY_TILE - y_bottom);
	}

	// Armed monsters must dirty their tiles to ensure RedrawMonsters doesn't cause flickering
	this->pTileImages[this->pRoom->ARRAYINDEX(pArmedMonster->wX, pArmedMonster->wY)].dirty = 1;

	DrawTileImage(blit, pDestSurface);

	if (bHasSword) {
		TileImageBlitParams weaponBlit(blit);
		weaponBlit.wCol = pArmedMonster->GetWeaponX();
		weaponBlit.wRow = pArmedMonster->GetWeaponY();
		weaponBlit.nAddColor = -1;
		weaponBlit.wTileImageNo = wSwordTI;
		DrawSwordFor(pArmedMonster, weaponBlit, pDestSurface);

		// The same for the sword, want to avoid flickering
		if (IS_COLROW_IN_DISP(weaponBlit.wCol, weaponBlit.wRow))
			this->pTileImages[this->pRoom->ARRAYINDEX(weaponBlit.wCol, weaponBlit.wRow)].dirty = 1;
	}

	TileImageBlitParams::resetDisplayArea();
}

//*****************************************************************************
void CRoomWidget::AddTemporalCloneNextMoveEffect(const CTemporalClone *pTC, const UINT frame)
{
	ASSERT(pTC);

	if (this->dwMovementStepsLeft)
		return;

	if (pTC->IsStunned() || pTC->bFrozen)
		return;

	const int command = pTC->getNextCommand();
	if (command == CMD_WAIT)
		return;

	//Determine potential new position and orientation of next move.
	int dx, dy;
	CMonster::GetCommandDXY(command, dx, dy);
	if (!this->pRoom->IsValidColRow(pTC->wX + dx, pTC->wY + dy))
		return;
	CMoveCoord coord(pTC->wX, pTC->wY, nGetO(dx, dy));

	//show one effect for this entity at a time
	const UINT wDisplayX = this->x + coord.wX * CBitmapManager::CX_TILE;
	const UINT wDisplayY = this->y + coord.wY * CBitmapManager::CY_TILE;
	if (this->pMLayerEffects->ContainsEffectOfTypeAt(ETEMPORALMOVE, wDisplayX, wDisplayY))
		return;

	UINT newO = pTC->wO;
	switch (command)
	{
		case CMD_C:
			newO = nNextCO(newO);
			break;
		case CMD_CC:
			newO = nNextCCO(newO);
			break;
		default: break;
	}

	const UINT logicalIdentity = pTC->GetIdentity();
	const UINT apparentIdentity = getApparentIdentity(logicalIdentity);
	const UINT wTI = GetEntityTile(apparentIdentity, logicalIdentity, newO, frame);

	CTemporalMoveEffect *pTME = new CTemporalMoveEffect(this, coord, wTI, bIsBumpCommand(command));
	AddMLayerEffect(pTME);

	if (pTC->HasSword()) {
		coord.wX += nGetOX(newO);
		coord.wY += nGetOY(newO);
		const UINT wSwordTI = GetSwordTileFor(pTC, newO, logicalIdentity);
		pTME = new CTemporalMoveEffect(this, coord, wSwordTI, bIsBumpCommand(command),
				EFFECTLIB::EGENERIC); //don't prevent another similar effect from originating here
		AddMLayerEffect(pTME);
	}
}

//*****************************************************************************
UINT CRoomWidget::getApparentIdentity(const UINT logicalIdentity) const
{
	UINT apparentIdentity = logicalIdentity;
	if (this->pCurrentGame && this->pCurrentGame->pHold)
		apparentIdentity = this->pCurrentGame->pHold->getRoleForLogicalIdentity(logicalIdentity);
	return apparentIdentity;
}

//*****************************************************************************
void CRoomWidget::DrawRockGiant(
//Draws RockGiant monster.
//
//Params:
	const CMonster *pMonster,    //(in)   Pointer to splitter monster.
	const bool /*bDrawRaised*/,    //(in)   never raised
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	ASSERT(pMonster->wType == M_ROCKGIANT);

	//Calculate animation offset.
	UINT wXOffset = 0, wYOffset = 0;
	if (this->dwMovementStepsLeft)
	{
		wXOffset = (pMonster->wPrevX - pMonster->wX) * this->dwMovementStepsLeft;
		wYOffset = (pMonster->wPrevY - pMonster->wY) * this->dwMovementStepsLeft;
	}
	const UINT wFrame = this->pTileImages[this->pRoom->ARRAYINDEX(pMonster->wX, pMonster->wY)].animFrame;

	const UINT wTopLeftTI = GetTileImageForEntity(M_ROCKGIANT, pMonster->wO, wFrame);
	TileImageBlitParams blit(pMonster->wX, pMonster->wY, wTopLeftTI, wXOffset, wYOffset, bMoveInProgress);
	DrawTileImage(blit, pDestSurface);

	UINT wI = 0;
	for (MonsterPieces::const_iterator piece = pMonster->Pieces.begin();
			piece != pMonster->Pieces.end(); ++piece, ++wI)
	{
		ASSERT(this->pRoom->IsValidColRow((*piece)->wX, (*piece)->wY));
		blit.wCol = (*piece)->wX;
		blit.wRow = (*piece)->wY;
		blit.wTileImageNo = GetTileImageForRockGiantPiece(wI, pMonster->wO, wFrame);
		DrawTileImage(blit, pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::DrawSwordFor(
//Draw a sword for the given monster.
//
//Params:
	const CMonster *pMonster,    //(in)   Pointer to monster.
	const UINT wType,            //monster type
	TileImageBlitParams& blit,
	SDL_Surface *pDestSurface)
{
	ASSERT(IsValidOrientation(pMonster->wO));
	blit.wTileImageNo = GetSwordTileFor(pMonster, pMonster->wO, wType);

	DrawSwordFor(pMonster, blit, pDestSurface);
}

//*****************************************************************************
UINT CRoomWidget::GetSwordTileFor(const CMonster *pMonster, const UINT wO, const UINT wType) const
{
	ASSERT(pMonster);
	UINT wMonsterType = wType;
	UINT wSwordTI = TI_UNSPECIFIED;

	//Get optional custom sword tile.
	if (this->pCurrentGame)
	{
		//Currently, supported for characters and player clones only.
		ASSERT(this->pCurrentGame->pHold);
		HoldCharacter *pCustomChar = NULL;
		switch (pMonster->wType)
		{
			case M_CHARACTER:
			{
				const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
				wMonsterType = pCharacter->wLogicalIdentity;
			}
			break;
			case M_CLONE:
			case M_TEMPORALCLONE:
			{
				CSwordsman& player = this->pCurrentGame->swordsman;
				wMonsterType = player.wIdentity;
			}
			break;
			default: break;
		}
	}

	//Calculate monster's default sword tile.
	return GetSwordTile(wMonsterType, wO, pMonster->GetWeaponType());
}


//*****************************************************************************
UINT CRoomWidget::GetSwordTileFor(const UINT wMonsterType, const UINT wO, const UINT wWeaponType) const
{
	UINT wSwordTI = TI_UNSPECIFIED;

	//Get optional custom sword tile.
	if (this->pCurrentGame && wMonsterType >= CUSTOM_CHARACTER_FIRST)
	{
		HoldCharacter* pCustomChar = this->pCurrentGame->pHold->GetCharacter(wMonsterType);
		wSwordTI = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles, GetCustomTileIndex(wO), SWORD_FRAME);
	}

	//Calculate monster's default sword tile.
	if (wSwordTI == TI_UNSPECIFIED)
		wSwordTI = GetSwordTile(wMonsterType, wO, wWeaponType);

	return wSwordTI;
}

//*****************************************************************************
void CRoomWidget::DrawSwordFor(
//Draw's a sword tile for the given monster.
//
//Params:
	const CMonster *pMonster,    //(in)   Pointer to monster.
	TileImageBlitParams& blit,
	SDL_Surface *pDestSurface) //(in)   Surface to draw to.
{
	ASSERT(pMonster);
	ASSERT(IS_COLROW_IN_DISP(pMonster->wX, pMonster->wY));

	//See if its sword square is within display.
	int nSgnX = 0, nSgnY = 0;
	if (this->dwMovementStepsLeft)
	{
		nSgnX = pMonster->wPrevX == pMonster->wX ? 0 :
				pMonster->wPrevX > pMonster->wX ? 1 : -1;
		nSgnY = pMonster->wPrevY == pMonster->wY ? 0 :
				pMonster->wPrevY > pMonster->wY ? 1 : -1;
	}

	//Sword isn't fully in display -- just draw it clipped.
	//(This is needed for when stepping onto room edge.)
	if (!IS_COLROW_IN_DISP(blit.wCol, blit.wRow) ||
			 !IS_COLROW_IN_DISP(blit.wCol + nSgnX, blit.wRow + nSgnY))
		blit.bClipped = true;

	DrawTileImage(blit, pDestSurface);
}

//*****************************************************************************
void CRoomWidget::DrawDoubleCursor(
//Draws a double cursor to a specified square of the room.
//
//Params:
	const UINT wCol, const UINT wRow,      //(in)   Destination square coords.
	SDL_Surface *pDestSurface) //(in)   Surface to draw to.
{
	ASSERT(IS_COLROW_IN_DISP(wCol, wRow));
	ASSERT(this->pCurrentGame);

	const CSwordsman& player = this->pCurrentGame->swordsman;
	const int xPixel = this->x + wCol * CX_TILE;
	const int yPixel = this->y + wRow * CY_TILE;
	const bool bObstacle = this->pRoom->
			DoesSquareContainDoublePlacementObstacle(wCol, wRow,
			player.wPlacingDoubleType);

	//Draw cursor.
	//Animate mimic cursor movement.
	static UINT wPrevCol = wCol, wPrevRow = wRow;
	if (!this->dwMovementStepsLeft || this->bFinishLastMovementNow)
	{
		wPrevCol = wCol;
		wPrevRow = wRow;
	}

	//Show illegal placement tile.
	if (bObstacle)
	{
		g_pTheBM->ShadeTile(xPixel,yPixel,Red,GetDestSurface());
		this->pTileImages[this->pRoom->ARRAYINDEX(wCol, wRow)].dirty = 1;
	} else {
		//Fade in and out.
		static Uint8 nOpacity = 160;
		static bool bFadeIn = false;
		if (nOpacity < 35)   //don't make too faint
			bFadeIn = true;
		else if (nOpacity > 180)   //don't make too dark
			bFadeIn = false;
		if (bFadeIn)
			nOpacity += 7;
		else
			nOpacity -= 5;

		//This double is not in the monster list yet.
		CMonster *pMonster = CMonsterFactory::GetNewMonster(MONSTERTYPE(player.wPlacingDoubleType));
		CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
		pDouble->SetCurrentGame(this->pCurrentGame);
		pDouble->wO = player.wO;
		pDouble->wX = wCol;
		pDouble->wY = wRow;
		pDouble->wPrevX = wPrevCol;
		pDouble->wPrevY = wPrevRow;
		pDouble->weaponType = player.GetActiveWeapon();
		pDouble->SetWeaponSheathed();
		DrawArmedMonster(pDouble, true, pDestSurface, true, nOpacity);
		delete pDouble;
	}

	//Draw bolt from player to cursor.
	static const UINT CX_TILE_HALF = CX_TILE / 2;
	static const UINT CY_TILE_HALF = CY_TILE / 2;
	const UINT wSX = player.wX; //always use player's actual position, since he's the only one who can place doubles
	const UINT wSY = player.wY;
	const int xS = this->x + wSX * CX_TILE + CX_TILE_HALF;
	const int yS = this->y + wSY * CY_TILE + CY_TILE_HALF;
	const int xC = xPixel + CX_TILE_HALF;
	const int yC = yPixel + CY_TILE_HALF;
	DrawBoltInRoom(xS, yS, xC, yC);
}

//*****************************************************************************************
void CRoomWidget::DrawSerpentBody(
// Starting after head, traverse and draw room tiles to the tail.
// Assumes a valid serpent.
//
//Params:
	const CMonster *pMonster,  //(in)   Pointer to CSerpent monster.
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	ASSERT(bIsSerpent(pMonster->wType));
	ASSERT(this->pRoom);

	//No animation of pieces.
	TileImageBlitParams blit(0, 0, 0, 0, 0, bMoveInProgress);
	const MonsterPieces& pieces = pMonster->Pieces;
	for (MonsterPieces::const_iterator piece = pieces.begin();
			piece != pieces.end(); ++piece)
	{
		blit.wCol = (*piece)->wX;
		blit.wRow = (*piece)->wY;
		blit.wTileImageNo = GetTileImageForSerpentPiece(pMonster->wType, (*piece)->wTileNo);
		DrawTileImage(blit, pDestSurface);
	}

	if (this->dwMovementStepsLeft)
	{
		//Erase where tail used to be.
		const CSerpent *pSerpent = DYN_CAST(const CSerpent*, const CMonster*, pMonster);
		this->pTileImages[this->pRoom->ARRAYINDEX(pSerpent->wOldTailX, pSerpent->wOldTailY)].dirty = 1;
	}
}

//*****************************************************************************************
void CRoomWidget::DrawGentryiiChain(
// Starting after head, traverse and draw the chain tiles.
// Assumes a valid gentryii tile ordering.
//
//Params:
	const CMonster *pMonster,  //(in)   Pointer to CGentryii monster.
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	ASSERT(this->pRoom);

	const CGentryii *pGentryii = DYN_CAST(const CGentryii*, const CMonster*, pMonster);
	UINT wPrevX = pGentryii->wX;
	UINT wPrevY = pGentryii->wY;

	const MonsterPieces& pieces = pMonster->Pieces;
	MonsterPieces::const_iterator pieces_end = pieces.end();
	for (MonsterPieces::const_iterator pieceIt = pieces.begin();
			pieceIt != pieces_end; ++pieceIt)
	{
		const CMonsterPiece& piece = **pieceIt;
		DrawGentryiiChainLink(pieceIt, pieces_end, wPrevX, wPrevY, pDestSurface, bMoveInProgress);

		wPrevX = piece.wX;
		wPrevY = piece.wY;
	}
}

//*****************************************************************************
void CRoomWidget::DrawGentryiiChainLink(
	MonsterPieces::const_iterator pieceIt,
	MonsterPieces::const_iterator pieces_end,
	UINT wPrevX, UINT wPrevY,
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool bMoveInProgress)
{
	//No movement animation of pieces.

	//Draw chain with a link to the previous tile.
	const CMonsterPiece& piece = **pieceIt;
	const UINT tile = GetTileImageForGentryiiPiece(wPrevX, wPrevY, piece.wX, piece.wY);
	TileImageBlitParams blit(piece.wX, piece.wY, tile, 0, 0, bMoveInProgress);
	DrawTileImage(blit, pDestSurface);

	//Draw diagonal corners on adjacent tiles to show that chain is solid across a diagonal.
	const int dx = piece.wX - wPrevX;
	const int dy = piece.wY - wPrevY;
	if (dx && dy) {
		if (dx > 0) {
			if (dy > 0) {
				blit.wCol = piece.wX;
				blit.wRow = piece.wY-1;
				blit.wTileImageNo = TI_GENTRYII_CORNER_SW;
				DrawTileImage(blit, pDestSurface);

				blit.wCol = piece.wX-1;
				blit.wRow = piece.wY;
				blit.wTileImageNo = TI_GENTRYII_CORNER_NE;
				DrawTileImage(blit, pDestSurface);
			} else {
				blit.wCol = piece.wX;
				blit.wRow = piece.wY+1;
				blit.wTileImageNo = TI_GENTRYII_CORNER_NW;
				DrawTileImage(blit, pDestSurface);

				blit.wCol = piece.wX-1;
				blit.wRow = piece.wY;
				blit.wTileImageNo = TI_GENTRYII_CORNER_SE;
				DrawTileImage(blit, pDestSurface);
			}
		} else {
			if (dy > 0) {
				blit.wCol = piece.wX;
				blit.wRow = piece.wY-1;
				blit.wTileImageNo = TI_GENTRYII_CORNER_SE;
				DrawTileImage(blit, pDestSurface);

				blit.wCol = piece.wX+1;
				blit.wRow = piece.wY;
				blit.wTileImageNo = TI_GENTRYII_CORNER_NW;
				DrawTileImage(blit, pDestSurface);
			} else {
				blit.wCol = piece.wX;
				blit.wRow = piece.wY+1;
				blit.wTileImageNo = TI_GENTRYII_CORNER_NE;
				DrawTileImage(blit, pDestSurface);

				blit.wCol = piece.wX+1;
				blit.wRow = piece.wY;
				blit.wTileImageNo = TI_GENTRYII_CORNER_SW;
				DrawTileImage(blit, pDestSurface);
			}
		}
	}

	//Optionally draw a link to the next tile.
	MonsterPieces::const_iterator nextPiece = pieceIt;
	if (++nextPiece != pieces_end) {
		blit.wCol = piece.wX;
		blit.wRow = piece.wY;
		blit.wTileImageNo = GetTileImageForGentryiiPiece((*nextPiece)->wX, (*nextPiece)->wY, piece.wX, piece.wY);
		DrawTileImage(blit, pDestSurface);
	}
}

//*****************************************************************************
void CRoomWidget::DrawSlayerWisp(
//Draw the wisp coming off of the Slayer.
//
//Params:
	const CMonster *pMonster,
	SDL_Surface *pDestSurface)
{
	static const UINT NUM_WISP_FRAMES = 4;
	static const UINT wispFrame[NUM_WISP_FRAMES] = {
		TI_WISP1, TI_WISP2, TI_WISP3, TI_WISP4
	};

	for (MonsterPieces::const_iterator piece = pMonster->Pieces.begin();
			piece != pMonster->Pieces.end(); ++piece)
	{
		CMonsterPiece *pPiece = *piece;
		ASSERT(this->pRoom->IsValidColRow(pPiece->wX, pPiece->wY));

		//Animate each piece.
		if (!pPiece->wTileNo)
			pPiece->wTileNo = 1 + RAND(NUM_WISP_FRAMES);
		if (this->bNextWispFrame)
		{
			if (++pPiece->wTileNo > NUM_WISP_FRAMES)
				pPiece->wTileNo = 1;
		}

		ASSERT(pPiece->wTileNo-1 < NUM_WISP_FRAMES);
		const TileImageBlitParams blit(pPiece->wX, pPiece->wY, wispFrame[pPiece->wTileNo-1]);
		DrawTileImage(blit, pDestSurface);

		//Must erase tile next frame.
		this->pTileImages[this->pRoom->ARRAYINDEX(pPiece->wX, pPiece->wY)].dirty = 1;
	}
}

//*****************************************************************************
//Determines how far light will be cast and what intensity it has at this square.
//If distance limit has not been reached, then light continues from square (wX,wY),
//according to direction light is being cast.
//
//As a performance optimization, objects casting shadow (based on room tile types)
//are considered and tileLight is populated with annotations of where shadows fall,
//as follows:
//
//L_Dark: Where a room tile creates a full occlusion of more distant tiles in
//a given direction, evaluation of more distant tiles can be skipped because
//this light will not fall anywhere on them.
//
//L_Light: Where no occlusion occurs, the 3-D room geometry does not need to
//be consulted to determine which parts of the tile this light shines on.
//
//L_Partial: When there is a partial occlusion, then the 3-D model needs to be
//consulated for potential intersections as multiple rays are cast from the
//light source to points on the tile (called "supersampling" below). Light
//is only added at points where no intersection from source to destination is found.
//
//The amount of light cast onto this tile is noted, to propagate this information
//to subsequent tiles processed further along this direction from the source.
void CRoomWidget::CastLightOnTile(
	const UINT wX, const UINT wY,    //(in) square to place light effect
	const float fLightSourceZTileElev, //(in) Z-elevation of tile that light source is on
	const PointLightObject& light,   //(in) light source
	const bool bGeometry)     //if true (default), take room geometry into account, otherwise ignore
{
	if (!this->pRoom->IsValidColRow(wX, wY))
		return;

	const float fX = light.location.x();
	const float fY = light.location.y();
	const int nSX = int(fX);
	const int nSY = int(fY);

	const int dx = (int)wX - nSX, dy = (int)wY - nSY;
	const int abs_dx = abs(dx);
	const int abs_dy = abs(dy);
	ASSERT(abs_dx <= MAX_LIGHT_DISTANCE);
	ASSERT(abs_dy <= MAX_LIGHT_DISTANCE);

	const float f_dx = float(int(wX))+0.5f - fX; //center (wX,wY)
	const float f_dy = float(int(wY))+0.5f - fY;

	//Consider light direction to perform simple optimizations.
	const bool bCenter = !dx && !dy;
	const bool bAxial = !dx || !dy;
	const bool bCorner = abs_dx == abs_dy;

	//determine one coord closer to the light source
	int closerDX = dx;
	if (dx>0) {
		--closerDX;
	} else if (dx<0) {
		++closerDX;
	}
	int closerDY = dy;
	if (dy>0) {
		--closerDY;
	} else if (dy<0) {
		++closerDY;
	}

	LightedType nextClosestTile = L_Light;
	if (bGeometry)
	{
		nextClosestTile = tileLight[MAX_LIGHT_DISTANCE + closerDX][MAX_LIGHT_DISTANCE + closerDY];
		if (!bCenter && nextClosestTile == L_Dark)
		{
			//Light fully blocked by closer tiles means no light shows here.
			if (bAxial || bCorner)
			{
				tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Dark;
				return;
			}

			const bool bHoriz = abs_dx > abs_dy;
			if (tileLight[MAX_LIGHT_DISTANCE + (bHoriz?closerDX:dx)][MAX_LIGHT_DISTANCE + (bHoriz?dy:closerDY)] == L_Dark)
			{
				tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Dark;
				return;
			}
		}
	}

	const float fElevAtLight = bGeometry ? fLightSourceZTileElev : 0.0f;
	const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
	float fElev = getTileElev(wOTile);

	//Is there an obstruction here?
	bool bWallShine = false;
	if (fElevAtLight <= 0.0f && fElev > 0.0f)
	{
		if (bIsDoor(wOTile))
		{
			tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Dark;
			return; //don't show light on door faces at all
		}

		//this wall actually only extends partly back -- so light can shine behind it (in the occluded area)
		bool bNorthernWall = bGeometry ? getTileElev(wX,wY-1) <= 0.0f : true;

		const UINT wTI = this->pTileImages[this->pRoom->ARRAYINDEX(wX,wY)].o;
		switch (wTI)
		{
			case TI_WALL_SW: case TI_WALL_SW2: case TI_WALL_SE: case TI_WALL_SE1:
			case TI_WALL_NSE: case TI_WALL_SWE: case TI_WALL_NSW: case TI_WALL_NSWE: case TI_WALL_NS:
			case TI_WALL_S: case TI_WALL_S1: case TI_WALL_S2: case TI_WALL_S3:
			case TI_WALL_BSW: case TI_WALL_BSW2: case	TI_WALL_BSE: case	TI_WALL_BSE1:
			case TI_WALL_BNSE: case TI_WALL_BSWE: case TI_WALL_BNSW: case TI_WALL_BNSWE: case TI_WALL_BNS:
			case TI_WALL_BS: case TI_WALL_BS1: case TI_WALL_BS2: case TI_WALL_BS3:
			case TI_WALL_HSW: case TI_WALL_HSW2: case	TI_WALL_HSE: case	TI_WALL_HSE1:
			case TI_WALL_HNSE: case TI_WALL_HSWE: case TI_WALL_HNSW: case TI_WALL_HNSWE: case TI_WALL_HNS:
			case TI_WALL_HS: case TI_WALL_HS1: case TI_WALL_HS2: case TI_WALL_HS3:
				bWallShine = dy < 0; //light propagating north shines on south wall faces
			break;
			default:
				if (!bGeometry) //don't ever shine wall lights on the top of walls
					return;

				if (!bNorthernWall)
				{
					tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Dark;
					return;	//don't show light on or past this wall at all
				}
				if (!dx)
				{
					tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Dark;
					return;  //light shining directly north or south won't expand further
				}
			break;
		}
		if (!bWallShine)
		{
			//Tile partially or completely obstructs light.
			tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] =
					bNorthernWall && dy >= 0 ? L_Partial : L_Dark;
			return;
		}

		//Prevent light shining on inner wall areas.
		if (!bGeometry && bWallShine)
		{
			if (getTileElev(wX,wY+1) > 0.0)
				return; //(x,y) is not really a south-facing wall
		}
	}

	//If we reached here, the tile probably has light on it.
	bool bFullyLit = true;
	//Did item on tile closer to light partially obstruct the light?
	bool bPartialOcclusion = nextClosestTile == L_PartialItem;
	tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] = L_Light;

	//Is there an object on the current tile that partially obstructs the light?
	bool bSphereItem = false;
	if (bGeometry)
		switch (this->pRoom->GetTSquare(wX, wY))
		{
			case T_ORB: case T_BOMB:
				bFullyLit = false; bPartialOcclusion = true; bSphereItem = true; break;
			default: break;
		}

	if (dynamic_cast<const SpotLightObject*>(&light)) {
		bFullyLit = false;
		bPartialOcclusion = true;
	} else if (bGeometry && bFullyLit && !bCenter)
	{
		//Light shining completely above walls is not occluded.
		if (!(light.location.z() > 1.0 && fElev > 0.0))
		{
			//If light is shining down from a wall-top to a lower tile, then
			//the wall will partially occlude the light.
			if (light.location.z() > 1.0 && fElev <= 0.0)
			{
				bFullyLit = false;
				bPartialOcclusion = true;
			}


			//If nothing on this tile stops light, and nothing before the tile stopped the light,
			//then this tile is fully lit also.
			if (tileLight[MAX_LIGHT_DISTANCE + closerDX][MAX_LIGHT_DISTANCE + closerDY] != L_Light ||
				 tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + closerDY] != L_Light ||
				 (tileLight[MAX_LIGHT_DISTANCE + closerDX][MAX_LIGHT_DISTANCE + dy] != L_Light &&
					!bWallShine))   //horizontally adjacent tiles won't occlude a wall shine
				bFullyLit = false; //otherwise there is probably only partial lighting on this tile
		}
	}

	//Distance in tiles from light source to top-left corner of this tile.
	static const float fLIGHT_SPT_MINUSONE = float(LIGHT_SPT_MINUSONE);
	static const float fEpsilon = 0.01f;
	static const float fHalfPlusEpsilon = 0.5f + fEpsilon; //slightly less than one-half to avoid ambiguous corners
	static const float fLightIncrement = (1.0f - 2*fEpsilon)/fLIGHT_SPT_MINUSONE; //slightly less than one to avoid hitting ambiguous tile edges
	const float dxTile = f_dx - fHalfPlusEpsilon;
	const float dyTile = f_dy - fHalfPlusEpsilon;

	//Light intensity.
	float fFullLight = fMaxLightIntensity[this->wDark];
	const float fDark = GetOverheadDarknessAt(wX, wY);
	if (fDark > 0.0f)
		fFullLight /= fDark; //shine brighter in specially marked dark tiles
	const float fMin = fFullLight / (1.1f * light.fMaxDistance);

	//Supersample the tile.
	const UINT wIndex = this->pRoom->ARRAYINDEX(wX,wY);
	const UINT wBaseLightIndex = wIndex * wLightValuesPerTile;
	int i,j;	//sub-tile granularity
	const bool positive_dx = dx>=0;
	const bool positive_dy = dy>=0;
	const int minI = positive_dx ? 0 : LIGHT_SPT_MINUSONE;
	const int minJ = positive_dy ? 0 : LIGHT_SPT_MINUSONE;
	const int maxI = minI==0 ? LIGHT_SPT : -1;
	const int maxJ = minJ==0 ? LIGHT_SPT : -1;
	const int incI = positive_dx ? 1 : -1;
	const int incJ = positive_dy ? 1 : -1;
	bool bIFirst, bJFirst = true;
	bool bSomeLight = false, bSomeDark = false;
	float fZ = max(0.0001f,fElev); //keep off of ground
	float dzSquared = 0.9f - fZ;    //the constant is generally the height of the light
	dzSquared *= dzSquared;
	for (j=minJ; j!=maxJ; j += incJ, bJFirst = false)
	{
		const float yOffset = j*fLightIncrement;

		//Except, that light shines on south wall faces.
		if (bWallShine)
		{
			if (j < LIGHT_SPT/2)
				continue;	//only show light on bottom-half of wall tile
			fZ = 2*(fElev - yOffset);
		}

		//Calculate distance of each sub-tile sample from light source (in pixels).
		const UINT wRowLightIndex = wBaseLightIndex + j*(LIGHT_SPT*LIGHT_BPP); //RGB
		const float dy = dyTile + yOffset;  //top-left corner of sub-tile
		const float dySquared = dy*dy;
		//add small offset to not check for light directly on the corner (and same for fX)
		const float fY = wY + (bWallShine ? 1.001f : yOffset + fEpsilon);
		const int prevJ = j-incJ;
		bIFirst = true;
		for (i=minI; i!=maxI; i += incI, bIFirst = false)
		{
			const float xOffset = i*fLightIncrement;
			const float dx = dxTile + xOffset;  //middle of sub-tile
			const int prevI = i-incI;

			if (!bFullyLit)
			{
				//Light probably doesn't shine completely here, so
				//find out where there's an occluding surface in the way.
				if (!bPartialOcclusion && !bIFirst && !bJFirst)
				{
					//When no occluding object is on this tile,
					//and we've already scanned an earlier row and column,
					//then certain deductions can be made:
					const UINT wCount = subtileLight[prevI][prevJ] + subtileLight[prevI][j] + subtileLight[i][prevJ];
					if (wCount == 3)
					{
						//Known: there is no shadow near this subtile, so there won't
						//be any shadow here either.
						goto AddLightToSubtile;
					}
					if (!wCount	&& fElevAtLight <= 0)
					{
						//Known: when the area in front of this subtile is in shadow,
						//and the light is not shining from above onto this tile,
						//then this further subtile is also in shadow.
						subtileLight[i][j] = L_Dark;
						bSomeDark = true;
						continue;
					}
				}

				//Cast a shadow ray.
				{
					float fZOffset = 0.0f;
					if (bSphereItem)
					{
						//Calculate point in space where light ray would hit sphere.
						static const float PiOverTwoOrbRad = 3.14159265359f / (2.0f*0.22f); //*orbRadius); //why does .22f work?  no idea...
						static const float fOffset = fEpsilon-0.5f;
						static const float fOrbRadiusSq = orbRadius * orbRadius;
						const float xOffsetEdge = xOffset+fOffset;
						const float yOffsetEdge = yOffset+fOffset;
						const float fRadSq = xOffsetEdge*xOffsetEdge + yOffsetEdge*yOffsetEdge;
						if (fRadSq <= fOrbRadiusSq)
							fZOffset = orbRadius * (1.0f + sin(PiOverTwoOrbRad*(orbRadius - sqrt(fRadSq))));
					}
					const Point here(wX + xOffset + fEpsilon, fY, fZ + fZOffset); //add small offset to not check directly on the tile corner
					if (!this->model.lightShinesAt(here, light))
					{
						subtileLight[i][j] = L_Dark;
						bSomeDark = true;
						continue;
					}
				}
AddLightToSubtile:
				subtileLight[i][j] = L_Light;
				bSomeLight = true;
			}

			//Calculate light intensity from source at center of this tile.
			//
			//Use linear falloff for light for better look than squared falloff.
			//fMin is the amount of light shown at the perimeter of the light.
			//It is subtracted so that it appears natural that the light has no
			//effect past its enforced maximum distance (wMaxDistance).
			const float fDistSquared = dx * dx + dySquared + dzSquared;
			float fLight = fFullLight / sqrt(fDistSquared) - fMin;

			//Add RGB light to tile.
			if (fLight > 0.0f)
			{
				//Emphasize wall shine.
				if (bWallShine)
					fLight *= fWallLightingMultiplier[this->wDark];

				const float sLight = float(fLight * 256.0f);
				const UINT wLightIndex = wRowLightIndex + i*LIGHT_BPP; //RGB
				const Color& color = light.color;
				LIGHTTYPE *lightPixel = this->lightMaps.pActiveLight + wLightIndex;
				UINT val;

				val = *lightPixel + UINT(color.r() * sLight);
				*lightPixel = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1);
				++lightPixel;

				val = *lightPixel + UINT(color.g() * sLight);
				*lightPixel = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1);
				++lightPixel;

				val = *lightPixel + UINT(color.b() * sLight);
				*lightPixel = val < LIGHTTYPE(-1) ? val : LIGHTTYPE(-1);
			}
		}
	}

	//Record what kind of light was actually mapped onto tile.
	if (bGeometry)
	{
		if (bSomeLight && !bSomeDark)
			bFullyLit = true;
		const LightedType tileLightType =
			tileLight[MAX_LIGHT_DISTANCE + dx][MAX_LIGHT_DISTANCE + dy] =
				bWallShine ? L_Dark : //Light shining on a wall is obstructed from further propagation.
				bFullyLit ? L_Light : //Nothing blocked light here.
				bPartialOcclusion ? L_PartialItem :  //An item partially blocked the light (and might not block it further out)
				bSomeLight ? (bSomeDark ? L_Partial : L_Light) :  //Was there some shadow here?
					L_Dark; //There was no light here.

		if (tileLightType == L_Partial || tileLightType == L_PartialItem)
			this->partialLightedTiles.insert(wX,wY); //blur light edge here

		//Set to the max value applied to this tile this frame
		//if the light is not on a wall or other elevated tile.
		//This is to facilitate light blurring when shadows cross floor-level tiles.
		if (fElev <= 0.0f && this->tileLightInfo.GetAt(wX,wY) < tileLightType)
			this->tileLightInfo.Set(wX,wY,tileLightType);
	}

	this->pActiveLightedTiles->insert(wX,wY);
	this->pTileImages[wIndex].dirty = 1;
}

//*****************************************************************************
void CRoomWidget::ProcessLightmap()
//Initialize the light display buffer.
//1. Light in room layer is blurred and applied to the display buffer.
//2. Ceiling lights are also added.
{
	//Render ceiling light map once when requested.
	if (!this->bCeilingLightsRendered)
	{
		//Reset ceiling light buffer.
		this->lightMaps.zero(this->lightMaps.psCeilingLight);
		if (!this->pRoom->tileLights.empty())
		{
			for (UINT wY = 0; wY < this->pRoom->wRoomRows; ++wY)
				for (UINT wX = 0; wX < this->pRoom->wRoomCols; ++wX)
				{
					const UINT lightVal = this->pRoom->tileLights.GetAt(wX,wY);
					if (bIsLightTileValue(lightVal))
					{
						//Light this ceiling tile.
						SetCeilingLight(wX,wY);
						this->pTileImages[this->pRoom->ARRAYINDEX(wX,wY)].dirty = 1;
					}
				}
		}
		this->bCeilingLightsRendered = true;
	}

	LIGHTTYPE *pCeilingDestBuffer = IsPlayerLightRendered() ?
			this->lightMaps.psRoomLight : this->lightMaps.psDisplayedLight;

	const UINT roomCols = this->pRoom->wRoomCols;
	const UINT roomRows = this->pRoom->wRoomRows;

	//1. Calculate room lighting buffer.
	if (this->lightedRoomTiles.empty() || this->partialLightedTiles.empty())
	{
		if (this->lightedRoomTiles.empty())
		{
			//If there are no room lights, reset buffer to display ceiling lights only.
			if (!this->pRoom->tileLights.empty() ||
					!this->pCurrentGame) //i.e. in the editor, always refresh
			{
				this->lightMaps.copy(pCeilingDestBuffer, this->lightMaps.psCeilingLight);
				return;
			}
			//If there are no ceiling lights either, then this buffer can be ignored.
		} else if (!IsPlayerLightRendered()) {
			//If there are room lights but they don't need to be smoothed, just display them.
			this->lightMaps.copy(pCeilingDestBuffer, this->lightMaps.psRoomLight);
		}
	} else {
		//Perform a low-pass filter on the room layer light map to reduce aliasing.
		//Write results to the display light buffer.
		LIGHTTYPE *pSrc = this->lightMaps.psRoomLight;
		LIGHTTYPE *pDest = this->lightMaps.psDisplayedLight; //result of blurring operation
			//The final display buffer may be used as the result buffer, since
			//a copy operation may be avoided if no more light needs to be layered

		const UINT wLastCol = roomCols-1;
		const UINT wLastRow = roomRows-1;
		for (UINT wY=0; wY<roomRows; ++wY)
		{
			//Process a row of room tiles.
			for (UINT wX=0; wX<roomCols; ++wX)
			{
				//Process a room tile.
				if (this->partialLightedTiles.has(wX,wY))
				{
					//Blurring the light on this tile will have a noticeable effect.

					//Whether there is light at the edges of the adjacent tiles to factor in.
					const bool bUp = wY > 0 && this->tileLightInfo.GetAt(wX,wY-1) != L_Dark;
					const bool bDown = wY < wLastRow && this->tileLightInfo.GetAt(wX,wY+1) != L_Dark;
					const bool bLeft = wX > 0 && this->tileLightInfo.GetAt(wX-1,wY) != L_Dark;
					const bool bRight = wX < wLastCol && this->tileLightInfo.GetAt(wX+1,wY) != L_Dark;

					LowPassLightFilter(pSrc, pDest, bLeft, bRight, bUp, bDown);
				} else {
					//Tile does not need to be blurred.

					//Initialize the display buffer with these values.
					memcpy(pDest, pSrc, wLightBytesPerTile);
				}
				pSrc += wLightValuesPerTile;
				pDest += wLightValuesPerTile;
			}
		}

		const UINT wSize = this->pRoom->CalcRoomArea() * wLightValuesPerTile;
		ASSERT(pSrc == this->lightMaps.psRoomLight + wSize);
		ASSERT(pDest == this->lightMaps.psDisplayedLight + wSize);

		//If more light must be added to the display buffer,
		//a copy of the final room light buffer must be preserved
		//so it doesn't have to be recalculated each turn.
		//Same thing for ceiling lights below.
		if (IsPlayerLightRendered())
			this->lightMaps.copy(this->lightMaps.psRoomLight, this->lightMaps.psDisplayedLight);
	}

	//2. Add ceiling light, either to the blurred room light buffer
	//or the display buffer if no other light will be added.
	if (!this->pRoom->tileLights.empty())
	{
		for (UINT wY = 0; wY < roomRows; ++wY)
			for (UINT wX = 0; wX < roomCols; ++wX)
			{
				const UINT lightVal = this->pRoom->tileLights.GetAt(wX,wY);
				if (bIsLightTileValue(lightVal))
				{
					//Add lighting from this tile.
					const UINT wStartIndex = this->pRoom->ARRAYINDEX(wX,wY) * wLightValuesPerTile;
					const LIGHTTYPE *pSrc = this->lightMaps.psCeilingLight + wStartIndex;
					LIGHTTYPE *pDest = pCeilingDestBuffer + wStartIndex;
					const LIGHTTYPE *pSrcTileEnd = pSrc + wLightValuesPerTile;
					while (pSrc != pSrcTileEnd)
					{
						*(pDest++) += *(pSrc++);
						*(pDest++) += *(pSrc++);
						*(pDest++) += *(pSrc++);
					}
				}
			}
	}
}

//*****************************************************************************
void CRoomWidget::LowPassLightFilter(
//Blurs a light map using a low-pass filter.
//
//Params:
	LIGHTTYPE *pSrc,  //(in) initial buffer
	LIGHTTYPE *pDest, //(out) blurred buffer
	const bool bLeft, const bool bRight, //(in) whether there is a tile that may
	const bool bUp, const bool bDown)    //be weighed in on this edge
const
{
	//Low-pass filter.
	static const UINT FILTER[9] = {
		0,5,0,
		5,12,5,
		0,5,0}; //kernel
	static const UINT FILTER_SUM = 32; //sum to a power of two for speed
	static const UINT OFFSET[9] = { //address offset to these locations in another tile
		0,LIGHT_SPT*LIGHT_BPP,0,
		(LIGHT_SPT*(LIGHT_SPT-1)+1)*LIGHT_BPP,0,(LIGHT_SPT*(LIGHT_SPT-1)+1)*LIGHT_BPP,
		0,LIGHT_SPT*LIGHT_BPP,0
	}; //!!diagonals not implemented

	const UINT wVertOffset = wLightValuesPerTile * (this->pRoom->wRoomCols-1);
	UINT i, j, k;

	for (j=0; j<LIGHT_SPT; ++j)
		for (i=0; i<LIGHT_SPT; ++i)
		{
			//Process one light pixel.
			for (k=3; k--; ) //one color channel each iteration
			{
				UINT dwSum = *pSrc * FILTER[4];  //center
				//left+right
				switch (i)
				{
					case 0:
						//mix in adjacent tile if not at edge, otherwise use the center (i.e. current) value
						dwSum += (bLeft ? *(pSrc-OFFSET[3]) : *pSrc) * FILTER[3];
						dwSum += *(pSrc+LIGHT_BPP) * FILTER[5];
					break;
					default:
						dwSum += *(pSrc-LIGHT_BPP) * FILTER[3]; //look in same tile
						dwSum += *(pSrc+LIGHT_BPP) * FILTER[5];
					break;
					case LIGHT_SPT-1:
						dwSum += *(pSrc-LIGHT_BPP) * FILTER[3];
						dwSum += (bRight ? *(pSrc+OFFSET[5]) : *pSrc) * FILTER[5];
					break;
				}
				//up+down
				switch (j)
				{
					case 0:
						dwSum += (bUp ? *(pSrc-OFFSET[1]-wVertOffset) : *pSrc) * FILTER[1];
						dwSum += *(pSrc+LIGHT_BPP*LIGHT_SPT) * FILTER[7];
					break;
					default:
						dwSum += *(pSrc-LIGHT_BPP*LIGHT_SPT) * FILTER[1];
						dwSum += *(pSrc+LIGHT_BPP*LIGHT_SPT) * FILTER[7];
					break;
					case LIGHT_SPT-1:
						dwSum += *(pSrc-LIGHT_BPP*LIGHT_SPT) * FILTER[1];
						dwSum += (bDown ? *(pSrc+OFFSET[7]+wVertOffset) : *pSrc) * FILTER[7];
					break;
				}

				dwSum /= FILTER_SUM;
				*(pDest++) = dwSum;
				++pSrc;
			}
		}
}

//*****************************************************************************
float CRoomWidget::getTileElev(const UINT i, const UINT j) const
//Return height info about this tile relevant for modeling.
{
	ASSERT(this->pRoom);
	if (!this->pRoom->IsValidColRow(i,j))
		return 0.0;

	return getTileElev(this->pRoom->GetOSquare(i,j));
}

float CRoomWidget::getTileElev(const UINT wOTile) const
{
	switch (wOTile)
	{
		case T_PIT: case T_PIT_IMAGE:
		case T_PLATFORM_P: //so light isn't drawn where platforms are rendered
			return -2.0f;

		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
		case T_WALL_B: case T_WALL_H: case T_WALL_M: case T_WALL_WIN:
			return 1.0f;

		case T_DOOR_Y: case T_DOOR_M: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B:
			return 0.75f;

		case T_WATER: case T_SHALLOW_WATER:
		case T_PLATFORM_W:
		case T_STAIRS_UP: case T_STAIRS:
			return 0.0f;

		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_STEP_STONE:
		case T_GOO: case T_HOT: case T_PRESSPLATE:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:	case T_DOOR_RO: case T_DOOR_BO:
		case T_FLOOR_ROAD: case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
		case T_FLOOR_M: case T_FLOOR: case T_FLOOR_IMAGE:
		default:
			return 0.0f;
	}
}

//*****************************************************************************
void CRoomWidget::modelVertTileface(
	const float elev, //height above/below floor
	const UINT i, const UINT j,  //where to render
	const bool bXAxis,  //whether tileface is in east-west (x) or north-south (y) direction
	const bool bNorthernWall)
{
	ASSERT(elev > 0.0);  //we're actually modeling a wall

	const Point p1((float)i, j + (bNorthernWall?fNorthWallYCoord:0.0f), 0),
		p2(i + (bXAxis?1.0f:0.0f), j + (bXAxis?(bNorthernWall?fNorthWallYCoord:0.0f):1.0f), elev);
	SceneRect *pTile = new SceneRect(p1,p2);
	this->model.addObject(pTile);
}

//*****************************************************************************
//Radiate out a light source onto the room tiles from its given position
//in the room, taking room geometry into account to identify areas of shadow.
//
//This is performed by casting light rays, first onto the tile where the light source
//originates, and then into each of the four cartesian quadrants in sequence.
//Each quadrant is processed distinctly, building a 3-D model
//of that area of the room only (for efficiency in applying the model)
//and then casting shadow rays to each tile of the area to determine where light
//shines and where shadow falls.
void CRoomWidget::PropagateLight(
	const float fSX, const float fSY,
	const float fZ, //elevation of tile light source is on
	const UINT tParam, 
	const bool bCenterOnTile, //[default=true]
	const Point& direction) //[default=(0,0,0), indicating everywhere
{
	if (bLightOff(tParam))
		return; //light is off

	const UINT wLightColor = calcLightType(tParam);

	//Center of tile (slightly offset, to avoid rounding instability)
	float fX = fSX;
	float fY = fSY;
	if (bCenterOnTile) {
		fX += 0.5f;
		fY += 0.501f+Y_LIGHT_OFFSET_KLUDGE;
	}

	const int nSX = int(fX);
	const int nSY = int(fY);
	ASSERT(this->pRoom->IsValidColRow(nSX,nSY));

	PointLightObject *pLight = NULL;
	if (direction.length() > 0.0f) {
		SpotLightObject *pSpotLight = new SpotLightObject(
				Point(fX, fY, max(0.0f, fZ) + 0.7f), //a little lower than the point light
				Color(lightMap[0][wLightColor], lightMap[1][wLightColor], lightMap[2][wLightColor]),
				Color()); //falloff unused
		pSpotLight->setDirection(direction);
		pSpotLight->cosineCutoff = this->pCurrentGame ? (this->pCurrentGame->playerLightType/100.0f) : 0.7f;
		pLight = pSpotLight;
	} else {
		pLight = new PointLightObject(
				Point(fX, fY, max(0.0f,fZ)+0.9f), //never in pit, and close to ceiling
				Color(lightMap[0][wLightColor], lightMap[1][wLightColor], lightMap[2][wLightColor]),
				Color()); //falloff unused
	}
	PointLightObject& light = *(dynamic_cast<PointLightObject*>(pLight));

	//Light's radius.
	const int nMaxDistance = 1+calcLightRadius(tParam);
	light.fMaxDistance = (float)nMaxDistance + 0.3f; //vertical component

	//The light source's tile itself.
	CastLightOnTile(nSX, nSY, fZ, light);

	//Cast light outward to the maximum range.
	//Process one quadrant at a time in order to maintain a compact area model.
	int dist, rad;
	//NW quadrant
	RenderRoomModel(nSX, nSY, nSX - nMaxDistance, nSY - nMaxDistance);  //model this quadrant
	for (dist=1; dist<=nMaxDistance; ++dist) {
		//1st. Axial.
		const int x = nSX - dist;
		const int y = nSY - dist;
		CastLightOnTile(x, nSY, fZ, light);
		CastLightOnTile(nSX, y, fZ, light);
		//2nd. Diagonal.
		for (rad=1; rad<dist; ++rad) {
			CastLightOnTile(x, nSY - rad, fZ, light);
			CastLightOnTile(nSX - rad, y, fZ, light);
		}
		//3rd. Corner.
		CastLightOnTile(x, y, fZ, light);
	}
	//NE quadrant
	RenderRoomModel(nSX, nSY, nSX + nMaxDistance, nSY - nMaxDistance);
	for (dist=1; dist<=nMaxDistance; ++dist) {
		const int x = nSX + dist;
		const int y = nSY - dist;
		CastLightOnTile(x, nSY, fZ, light);
		for (rad=1; rad<dist; ++rad) {
			CastLightOnTile(x, nSY - rad, fZ, light);
			CastLightOnTile(nSX + rad, y, fZ, light);
		}
		CastLightOnTile(x, y, fZ, light);
	}
	//SW quadrant
	RenderRoomModel(nSX, nSY, nSX - nMaxDistance, nSY + nMaxDistance);
	for (dist=1; dist<=nMaxDistance; ++dist) {
		const int x = nSX - dist;
		const int y = nSY + dist;
		CastLightOnTile(nSX, y, fZ, light);
		for (rad=1; rad<dist; ++rad) {
			CastLightOnTile(x, nSY + rad, fZ, light);
			CastLightOnTile(nSX - rad, y, fZ, light);
		}
		CastLightOnTile(x, y, fZ, light);
	}
	//SE quadrant
	RenderRoomModel(nSX, nSY, nSX + nMaxDistance, nSY + nMaxDistance);
	for (dist=1; dist<=nMaxDistance; ++dist) {
		const int x = nSX + dist;
		const int y = nSY + dist;
		for (rad=1; rad<dist; ++rad) {
			CastLightOnTile(x, nSY + rad, fZ, light);
			CastLightOnTile(nSX + rad, y, fZ, light);
		}
		CastLightOnTile(x, y, fZ, light);
	}

	delete pLight;
}

//*****************************************************************************
void CRoomWidget::PropagateLightNoModel(const int nSX, const int nSY, const UINT tParam)
//Radiate out a light source onto the room tiles from its given position in the room.
{
	ASSERT(this->pRoom->IsValidColRow(nSX,nSY));

	if (bLightOff(tParam))
		return; //light is off

	const UINT wLightColor = calcLightType(tParam);

	const float fZ = 0.0; //always on the floor
	//Center of tile (slightly offset, to avoid rounding instability)
	const float fX = nSX+0.5f, fY = nSY+0.501f+Y_LIGHT_OFFSET_KLUDGE;
	PointLightObject light(
			Point(fX, fY, max(0.0f,fZ)+0.9f), //never in pit, and close to ceiling
			Color(lightMap[0][wLightColor], lightMap[1][wLightColor], lightMap[2][wLightColor]),
			Color()); //falloff unused

	//Light's radius.
	const int nMaxDistance = 1+calcLightRadius(tParam);
	light.fMaxDistance = (float)nMaxDistance + 0.3f; //vertical component

	this->model.clear();

	//Cast light outward to the maximum range.
	int nXdist, nYdist;
	for (nYdist=-nMaxDistance; nYdist<=nMaxDistance; ++nYdist)
		for (nXdist=-nMaxDistance; nXdist<=nMaxDistance; ++nXdist)
			CastLightOnTile(nSX + nXdist, nSY + nYdist, fZ, light, false);
}

//*****************************************************************************
void CRoomWidget::ReduceJitter()
//Reduce the amount of jitter on each tile every so often
//so things don't shake forever.
{
	if (!this->bJitterThisFrame)
		return;

	const Uint32 now = SDL_GetTicks();
	const Uint32 elapsed = now - this->dwLastJitterReduction;
	if (elapsed < JITTER_REDUCTION_INTERVAL)
		return;

	this->dwLastJitterReduction = now;

	UINT wX, wY;
	for (wY = 0; wY < this->pRoom->wRoomRows; ++wY)
		for (wX = 0; wX < this->pRoom->wRoomCols; ++wX)
		{
			const UINT jitter = this->jitterInfo.GetAt(wX,wY);
			if (jitter)
				this->jitterInfo.Set(wX,wY,jitter-1);
		}
}

//*****************************************************************************
void CRoomWidget::RemoveHighlight()
//Reset tile marked for custom highlighting.
{
	this->pHighlitMonster = NULL;
	this->wHighlightX = this->wHighlightY = UINT(-1);
	this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);
	ResetUserLightsource();
}

void CRoomWidget::ResetUserLightsource()
{
	this->cursorLight.first = this->cursorLight.second = -1;
}

//*****************************************************************************
//Generate 3-D model of specified room area
//  (i.e. simple geometry of obstructing surfaces, like walls and orbs).
//This model is subsequently used to assess where shadows are cast,
//by checking for intersections of rays from the source to destination points
//with the room geometry.
void CRoomWidget::RenderRoomModel(const int nX1, const int nY1, const int nX2, const int nY2)
{
	int nMinX = min(nX1,nX2), nMinY = min(nY1,nY2);
	int nMaxX = max(nX1,nX2), nMaxY = max(nY1,nY2);
	if (nMinX < 0) nMinX = 0;
	if (nMinY < 0) nMinY = 0;
	if ((UINT)nMaxX >= this->pRoom->wRoomCols) nMaxX = this->pRoom->wRoomCols-1;
	if ((UINT)nMaxY >= this->pRoom->wRoomRows) nMaxY = this->pRoom->wRoomRows-1;

	this->model.init(nMinX, nMinY, nMaxX, nMaxY);

	UINT i,j;
	for (j=nMinY; j<=(UINT)nMaxY; ++j)
	{
		//Render a row.
		float maxElev, elev, northElev, westElev = getTileElev(nMinX-1, j);

		//Northern-facing walls (i.e. there's no more wall to the north past this wall tile)
		//extend only part of a tile back/north.
		bool bNorthernWall, bLastWallIsNorthern;

		for (i=nMinX; i<=(UINT)nMaxX; ++i)
		{
			//Get room tile type and matching texture.
			elev = getTileElev(i, j);

			const UINT wTTile = this->pRoom->GetTSquare(i, j);
			switch (wTTile)
			{
				case T_ORB:
				case T_BOMB:
				{
					Sphere *pOrb = new Sphere(Point(i+0.5f, j+0.5f,
							max(0.0f,elev) + orbRadius), orbRadius);
					this->model.addObject(pOrb);
				}
				break;
			}

			//west/east edge
			if (i>UINT(nMinX))
			{
				if (westElev != elev)
				{
					maxElev = max(westElev, elev);
					if (maxElev > 0.0)
					{
						bNorthernWall =
							(westElev > 0.0 && getTileElev(i-1,j-1) <= 0.0) ||
							(elev > 0.0 && getTileElev(i,j-1) <= 0.0);
						modelVertTileface(maxElev, i, j, false, bNorthernWall);
					}
				} else {
					//If this wall is northern facing and last one is not, or v.v.,
					//then a partial wall piece must be added.
					bLastWallIsNorthern = westElev > 0.0 && getTileElev(i-1,j-1) <= 0.0;
					bNorthernWall = elev > 0.0 && getTileElev(i,j-1) <= 0.0;
					if (bLastWallIsNorthern != bNorthernWall)
					{
						maxElev = max(westElev, elev);
						const Point p1((float)i, (float)j, 0),
							p2((float)i, j + fNorthWallYCoord, maxElev);
						SceneRect *pTile = new SceneRect(p1,p2);
						this->model.addObject(pTile);
					}
				}
			}

			//north/south edge
			if (j>UINT(nMinY))
			{
				northElev = getTileElev(i, j-1);
				if (northElev != elev)
				{
					maxElev = max(northElev, elev);
					if (maxElev > 0.0)
					{
						bNorthernWall = northElev <= 0.0;
						modelVertTileface(maxElev, i, j, true, bNorthernWall);
					}
				}
			}
			westElev = elev;
		}
	}

	this->model.ready();  //done adding to the model
}

//*****************************************************************************
void CRoomWidget::SetCeilingLight(const UINT wX, const UINT wY)
//Set light values on tile (x,y) in the ceiling light map.
{
	const float fLightMultiplier = fMaxLightIntensity[this->wDark] * 256.0f;
	ASSERT(this->pRoom->tileLights.GetAt(wX,wY));
	const UINT wLightIndex = this->pRoom->tileLights.GetAt(wX,wY);
	ASSERT(bIsLightTileValue(wLightIndex)); //this should be a light tile, not a dark tile
	const UINT wLightColor = calcLightType(wLightIndex-1);
	const LIGHTTYPE R = static_cast<LIGHTTYPE>(lightMap[0][wLightColor] * fLightMultiplier);
	const LIGHTTYPE G = static_cast<LIGHTTYPE>(lightMap[1][wLightColor] * fLightMultiplier);
	const LIGHTTYPE B = static_cast<LIGHTTYPE>(lightMap[2][wLightColor] * fLightMultiplier);

	//Add light on edges if another light tile is next that edge.
	bool bWestLight = true, bNorthLight = true, bEastLight = true, bSouthLight = true;
	UINT lightVal;
	if (wX)
	{
		lightVal = this->pRoom->tileLights.GetAt(wX-1,wY);
		bWestLight = bIsLightTileValue(lightVal);
	}
	if (wY)
	{
		lightVal = this->pRoom->tileLights.GetAt(wX,wY-1);
		bNorthLight = bIsLightTileValue(lightVal);
	}
	if (wX<this->pRoom->wRoomCols-1)
	{
		lightVal = this->pRoom->tileLights.GetAt(wX+1,wY);
		bEastLight = bIsLightTileValue(lightVal);
	}
	if (wY<this->pRoom->wRoomRows-1)
	{
		lightVal = this->pRoom->tileLights.GetAt(wX,wY+1);
		bSouthLight = bIsLightTileValue(lightVal);
	}

	LIGHTTYPE *psL = this->lightMaps.psCeilingLight +
			(wY * this->pRoom->wRoomCols + wX) * wLightValuesPerTile;
	bool bRender = true;
	for (UINT j=0; j<LIGHT_SPT; ++j)
	{
		if (j==0 && !bNorthLight)
		{
			psL += LIGHT_SPT*LIGHT_BPP; //advance to next row
			continue;
		}
		if (j==LIGHT_SPT_MINUSONE && !bSouthLight)
			break; //done
		for (UINT i=0; i<LIGHT_SPT; ++i)
		{
			if (i==0)
			{
				if (!bWestLight)
					bRender = false;
				//Handle west corner values.
				else if (j==0 && wX>0 && wY>0)
				{
					lightVal = this->pRoom->tileLights.GetAt(wX-1,wY-1);
					if (!bIsLightTileValue(lightVal))
						bRender = false;
				}
				else if (j==LIGHT_SPT_MINUSONE && wX>0 && wY<this->pRoom->wRoomRows-1)
				{
					lightVal = this->pRoom->tileLights.GetAt(wX-1,wY+1);
					if (!bIsLightTileValue(lightVal))
						bRender = false;
				}
			}
			else if (i==LIGHT_SPT_MINUSONE)
			{
				if (!bEastLight)
					bRender = false;
				//Handle east corner values.
				else if (j==0 && wX<this->pRoom->wRoomCols-1 && wY>0)
				{
					lightVal = this->pRoom->tileLights.GetAt(wX+1,wY-1);
					if (!bIsLightTileValue(lightVal))
						bRender = false;
				}
				else if (j==LIGHT_SPT_MINUSONE && wX<this->pRoom->wRoomCols-1 &&
						wY<this->pRoom->wRoomRows-1)
				{
					lightVal = this->pRoom->tileLights.GetAt(wX+1,wY+1);
					if (!bIsLightTileValue(lightVal))
						bRender = false;
				}
			}

			if (!bRender)
			{
				psL += LIGHT_BPP; //advance to next pixel
				bRender = true;   //reset for next pixel
			} else {
				*(psL++) = R;
				*(psL++) = G;
				*(psL++) = B;
			}
		}
	}
}

//*****************************************************************************
bool CRoomWidget::SkyWillShow() const
//Returns: true if room contains some tile that shows the sky/ceiling
{
	for (UINT wRow = this->pRoom->wRoomRows; wRow--; )
		for (UINT wCol = this->pRoom->wRoomCols; wCol--; )
		{
			//These tiles show open sky.
			const UINT wOSquare = this->pRoom->GetOSquare(wCol, wRow);
			if (bIsWater(wOSquare) || bIsSteppingStone(wOSquare) || wOSquare == T_PLATFORM_W)
				return true;
		}

	return false;
}

//*****************************************************************************
void CRoomWidget::SynchRoomToCurrentGame()
//Keep room object synched to current game's room object.
{
	if (this->pCurrentGame)
		if (this->pCurrentGame->pRoom != this->pRoom)
		{
			SyncRoomPointerToGame(this->pCurrentGame);
			LoadRoomImages();
		}
}

void CRoomWidget::SyncRoomPointerToGame(CCurrentGame* pGame)
{
	ASSERT(pGame);
	this->pRoom = pGame->pRoom;
}

//*****************************************************************************
bool CRoomWidget::UpdateDrawSquareInfo(
//Update square drawing information arrays for a room.
//
//Params:
	const CCoordSet *pSet,	//(in) squares which were replotted [default=NULL]
	const CCoordSet *pGeometryChanges) //(in) tiles where room geometry changed, which affects lighting [default=NULL]
//Returns:
//True if successful, false if not (out of memory error).
{
	if (!this->pRoom)
		return true;

	const UINT wSquareCount = this->pRoom->CalcRoomArea();
	UINT wIndex;

	//If the square count changed, then I need to realloc arrays.
	if (wSquareCount != this->dwLastDrawSquareInfoUpdateCount)
	{
		if (!SetupDrawSquareInfo())
			return false;
		this->dwLastDrawSquareInfoUpdateCount = wSquareCount;
	}

	//Set tile image elements of arrays.
	TileImages *pTI = this->pTileImages;
	const char *pucO = this->pRoom->pszOSquares;
	const char *pucF = this->pRoom->pszFSquares;
	const RoomObject *const *pucT = this->pRoom->tLayer;
	const UINT wRows = this->pRoom->wRoomRows, wCols = this->pRoom->wRoomCols;
	UINT wTileImage, wLightVal, wTile;
	EDGETYPE drawEdge;
	bool bHalfWall;

	//Reset complex tile masks / light values.
	//Note: This can't be done along with the shadow and recalc arrays since these values
	//are not added to the tiles in order.
	for (wIndex = 0; wIndex<wSquareCount; ++wIndex)
		this->pTileImages[wIndex].shadowMasks.clear();

	//If room viewing method changed, dirty relevant tiles.
	if (this->bLastVision != IsShowingBetterVision())
	{
		this->bLastVision = IsShowingBetterVision();

		//Dirty all tarstuff.
		for (wIndex = wSquareCount; wIndex--; ) {
			const RoomObject *tObj = pucT[wIndex];
			if (tObj && bIsTar(tObj->tile))
				pTI[wIndex].dirty = 1;
		}
	}

	//Determine which tiles needed to be recalculated.
	CCoordIndex recalc(this->pRoom->wRoomCols, this->pRoom->wRoomRows, this->bAllDirty || !pSet ? 1 : 0);
	if (pSet && !this->bAllDirty)
	{
		for (CCoordSet::const_iterator tile = pSet->begin(); tile != pSet->end(); ++tile)
		{
			//Mark this tile for redraw.
			const int nX = tile->wX, nY = tile->wY;
			pTI[this->pRoom->ARRAYINDEX(nX, nY)].dirty = 1;

			//For each plotted tile, all adjacent tiles within two units possibly
			//need to be refreshed (rerendered).  If this changes in the future,
			//a more sophisticated marking scheme might need to be used.
			int nI, nJ;
			for (nJ = nY-2; nJ <= nY+2; ++nJ)
				for (nI = nX-2; nI <= nX+2; ++nI)
					if (this->pRoom->IsValidColRow(nI, nJ))
						recalc.Add(nI,nJ);
		}
	}

	//If the room geometry possibly changed adjacent to where lights shine,
	//then refresh the room light model.
	//(This set is a subset of 'pSet'.)
	if (pGeometryChanges && !this->bAllDirty)
	{
		for (CCoordSet::const_iterator tile = pGeometryChanges->begin(); tile != pGeometryChanges->end(); ++tile)
		{
			const int nX = tile->wX, nY = tile->wY;

			//Tiles below ground level don't ever affect lighting.
			UINT wOTile = this->pRoom->GetOSquare(nX,nY);
			const bool bNotPit = !(bIsPit(wOTile) || bIsWater(wOTile) || bIsThinIce(wOTile) ||
					bIsSteppingStone(wOTile) || bIsPlatform(wOTile));
			if (bNotPit)
			{
				int nI, nJ;
				for (nJ = nY-1; nJ <= nY+1; ++nJ)
					for (nI = nX-1; nI <= nX+1; ++nI)
						if (this->pRoom->IsValidColRow(nI, nJ))
						{
							//Tiles below ground level don't ever affect lighting.
							wOTile = this->pRoom->GetOSquare(nI,nJ);
							if (!(bIsPit(wOTile) || bIsWater(wOTile) || bIsThinIce(wOTile) ||
								bIsSteppingStone(wOTile) || bIsPlatform(wOTile)) &&
									//If light was drawn here last turn, then
									//this plot might have affected the light casting.
									(this->lightedPlayerTiles.has(nI,nJ) || this->lightedRoomTiles.has(nI,nJ)))
								this->bRenderRoomLight = true;
						}
			}
		}
	}

	//Re-model the room lighting when something has changed.
	const bool bRenderLights = this->bRenderRoomLight && IsLightingRendered();
	if (bRenderLights)
	{
		//Erase old room lighting and prepare for new lighting.
		this->lightMaps.zero(this->lightMaps.psRoomLight);
		if (this->bAllDirty)
			this->lightMaps.zero(this->lightMaps.psPlayerLight); //reset the old player light buffer

		//Refresh tiles that used to be lit.
		if (!this->lightedRoomTiles.empty())
		{
			if (!this->bAllDirty)
			{
				//Only need to refresh tiles that used to have light drawn on them.
				for (CCoordSet::const_iterator tile = this->lightedRoomTiles.begin();
						tile != this->lightedRoomTiles.end(); ++tile)
					this->pTileImages[this->pRoom->ARRAYINDEX(tile->wX,tile->wY)].dirty = 1;
			}

			this->lightedRoomTiles.clear();
			this->partialLightedTiles.clear();
			this->tileLightInfo.Clear();
			ResetJitter();
		}

		this->lightMaps.pActiveLight = this->lightMaps.psRoomLight; //write to room light buffer
		this->pActiveLightedTiles = &this->lightedRoomTiles;
	}

	wIndex = 0;
	for (UINT wRow = 0; wRow < wRows; ++wRow)
	{
		for (UINT wCol = 0; wCol < wCols; ++wCol, ++wIndex)
		{
			const RoomObject *tObj = *pucT;
			const UINT tTile = tObj ? tObj->tile : RoomObject::emptyTile();

			//Retain old TILEINFO data.  More tiles might be dirtied below.
			if (recalc.Exists(wCol, wRow))
			{

			//Calculate aesthetic tile edges.
			//If existence of an edge changes, tile must be redrawn.
			if (wRow > 0)
			{
				drawEdge = CalcEdge(*pucO, *(pucO - wCols),N);
				if (drawEdge != pTI->edges.north)
				{
					pTI->dirty = 1;
					pTI->edges.north = drawEdge;
					//Touch up corners.
					if (wCol > 0) (pTI-1)->dirty = 1;
					if (wCol < wCols-1) (pTI+1)->dirty = 1;
				}
			}

			if (wCol > 0)
			{
				drawEdge = CalcEdge(*pucO,*(pucO - 1),W);
				if (drawEdge != pTI->edges.west)
				{
					pTI->dirty = 1;
					pTI->edges.west = drawEdge;
					//Touch up corners.
					if (wRow > 0) (pTI-wCols)->dirty = 1;
					if (wRow < wRows-1) (pTI+wCols)->dirty = 1;
				}
			}

			if (wCol < wCols-1)
			{
				drawEdge = CalcEdge(*pucO,*(pucO + 1),E);
				if (drawEdge != pTI->edges.east)
				{
					pTI->dirty = 1;
					pTI->edges.east = drawEdge;
					//Touch up corners.
					if (wRow > 0) (pTI-wCols)->dirty = 1;
					if (wRow < wRows-1) (pTI+wCols)->dirty = 1;
				}
			}

			if (wRow < wRows-1)
			{
				drawEdge = CalcEdge(*pucO,*(pucO + wCols),S);
				if (drawEdge != pTI->edges.south)
				{
					pTI->dirty = 1;
					pTI->edges.south = drawEdge;
					//Touch up corners.
					if (wCol > 0) (pTI-1)->dirty = 1;
					if (wCol < wCols-1) (pTI+1)->dirty = 1;
				}
			}

			//Calculate o-layer tiles
			//If tile changes, it must be redrawn.
			wTile = *pucO;

			//Tiles that aren't drawn to the room snapshot because they are animated.
			if (bIsPlatform(wTile))
				wTile = this->pRoom->coveredOSquares.GetAt(wCol, wRow);

			wTileImage = GetTileImageForTileNo(wTile);
			if (wTileImage == CALC_NEEDED)
				wTileImage = CalcTileImageForOSquare(this->pRoom, wCol, wRow);
			if (wTileImage != pTI->o)
			{
				pTI->dirty = 1;

				//Update tiles with something drawn south of/underneath
				if (wRow < wRows-1) {
					const UINT oldTileImage = pTI->o;
					//Must "undraw" the trapdoor/bridge edge on the tile below when it falls.
					switch (oldTileImage) {
						case TI_TRAPDOOR: case TI_TRAPDOOR2:
						case TI_BRIDGE: case TI_BRIDGE_H: case TI_BRIDGE_V:
							(pTI+wCols)->dirty = 1;
							break;
						default:
							if ((TI_WALL_BNW <= oldTileImage && oldTileImage <= TI_WALL_B15) ||
									(TI_WALL_HNW <= oldTileImage && oldTileImage <= TI_WALL_H15)) {
								//Remove any reflection of the crumbly wall in water.
								(pTI+wCols)->dirty = 1;
							}
							break;
					}
				}

				pTI->o = wTileImage;
			}

			//Calculate whether special textures will be drawn on tile.
			EDGES& edges = pTI->edges;
			bHalfWall = ((
				(wTileImage >= TI_WALL_SW && wTileImage <= TI_WALL_NS) ||
				(wTileImage >= TI_WALL_S && wTileImage <= TI_WALL_S3)  ||
				(wTileImage >= TI_WALL_BSW && wTileImage <= TI_WALL_BNS) ||
				(wTileImage >= TI_WALL_BS && wTileImage <= TI_WALL_BS3) ||
				(wTileImage >= TI_WALL_HSW && wTileImage <= TI_WALL_HNS) ||
				(wTileImage >= TI_WALL_HS && wTileImage <= TI_WALL_HS3) ) &&
					GetWallTypeAtSquare(this->pRoom, wCol, wRow + 1) == WALL_INNER);
			if (bHalfWall != edges.bHalfWall)
			{
				pTI->dirty = 1;
				edges.bHalfWall = !edges.bHalfWall;
			}

			//Calculate pit edge info.
			if (bIsPit(*pucO) || *pucO == T_PLATFORM_P)
			{
				const UINT wPitX = edges.wPitX, wPitY = edges.wPitY, wPitRem = edges.wPitRemaining;
				CalcTileCoordForPit(this->pRoom, wCol, wRow, &edges);
				if (wPitX != edges.wPitX || wPitY != edges.wPitY || wPitRem != edges.wPitRemaining)
					pTI->dirty = 1;
			}

			//Calculate f-layer tiles
			//If tile changes, it must be redrawn.
			wTileImage = GetTileImageForTileNo(*pucF);
			ASSERT(wTileImage != CALC_NEEDED);
			if (wTileImage != pTI->f)
			{
				pTI->dirty = 1;
				pTI->f = wTileImage;
			}

			//Calculate t-layer tiles
			//If tile changes, it must be redrawn.
			wTileImage = GetTileImageForTileNo(tTile);
			if (wTileImage == CALC_NEEDED)
				wTileImage = CalcTileImageForTSquare(this->pRoom, wCol, wRow);
			if (wTileImage != pTI->t)
			{
				pTI->dirty = 1;
				pTI->t = wTileImage;
			}

			wTileImage = GetTileImageForTileNo(tObj ? tObj->coveredTile : RoomObject::emptyTile());
			if (wTileImage == CALC_NEEDED)
				wTileImage = CalcTileImageForCoveredTSquare(this->pRoom, wCol, wRow);
			if (wTileImage != pTI->tCovered)
			{
				pTI->dirty = 1;
				pTI->tCovered = wTileImage;
			}

			}	//recalc

			//Keep track of where wall shadows are being cast.
			//If shadow changes, tile must be redrawn.
			if (this->wDark || !bShowsShadow(*pucO))	//no shadows in dark rooms
			{
				//These objects show no shadow.
				if (pTI->wallShadow != TI_UNSPECIFIED)
				{
					pTI->dirty = 1;
					pTI->wallShadow = TI_UNSPECIFIED;
				}
			} else {
				//Shadows can fall on this tile.
				const UINT wShadow = CalcTileImagesForWallShadow(this->pRoom, wCol, wRow);
				if (wShadow != TI_UNSPECIFIED)
					this->pTileImages[wIndex].shadowMasks.push_back(wShadow);
				if (wShadow != pTI->wallShadow)
				{
					pTI->dirty = 1;
					pTI->wallShadow = wShadow;
				}
			}

			//Compile set of all obstacle shadows.
			if (tTile == T_OBSTACLE && !this->wDark)	//no shadows in dark rooms
				AddObstacleShadowMask(wCol,wRow);

			//Light sources.
			//T-layer lights and wall lights are handled here.
			if (bRenderLights)
			{
				if (tTile == T_BEACON) {
					static const UINT wBeaconLightRadius = 2;
					static const UINT wLightParam = (wBeaconLightRadius-1)*NUM_LIGHT_TYPES + 4; //light red
					const float fZ = getTileElev(*pucO);
					PropagateLight(float(wCol), float(wRow), fZ, wLightParam);
				}
				if (bIsLight(tTile)) {
					const float fZ = getTileElev(*pucO);
					PropagateLight(float(wCol), float(wRow), fZ, this->pRoom->GetTParam(wCol, wRow));
				}

				wLightVal = this->pRoom->tileLights.GetAt(wCol, wRow);
				if (bIsWallLightValue(wLightVal))
					PropagateLightNoModel(wCol, wRow, wLightVal);
			}

			//Monster tiles might change, but most of these are taken care of
			//in DirtySpriteTiles() as they move.  Cases not taken care of
			//are (1) when a monster is killed without an effect to dirty its tile,
			//or (2) when an entire long monster (e.g. serpent) is destroyed at once.

			//Advance to next square.
			++pTI;
			++pucO;
			++pucF;
			++pucT;
		}
	}

	//I'm expecting pointers to have traversed entire size of their arrays--
	//no more, no less.
	ASSERT(pTI == this->pTileImages + wSquareCount);
	ASSERT(pucO == this->pRoom->pszOSquares + wSquareCount);
	ASSERT(pucF == this->pRoom->pszFSquares + wSquareCount);
	ASSERT(pucT == this->pRoom->tLayer + wSquareCount);

	if (bRenderLights)
	{
		//Done rendering room lighting.  Prepare for display.
		ProcessLightmap();
		this->bRenderPlayerLight = IsPlayerLightRendered(); //add player light to room display also
		this->bRenderRoomLight = false;

		//If the room currently has no lights turned on, but there were lights
		//that were just turned off, then 'this->psDisplayedLight' will be used for
		//rendering the player light directly.  In this case, we want to reset
		//the light values on the display buffer where the lights that just got
		//turned off used to be shining.
		const bool bRoomHasLighting = !this->pRoom->tileLights.empty() || !this->lightedRoomTiles.empty();
		if (!bRoomHasLighting && this->bRenderPlayerLight)
		{
			CCoordSet clearedTiles;
			for (CCoordSet::const_iterator tile = this->pRoom->disabledLights.begin();
					tile != this->pRoom->disabledLights.end(); ++tile)
			{
				//Just reset everywhere within the disabled light's range.
				const UINT wX = tile->wX, wY = tile->wY;
				const BYTE tParam = this->pRoom->GetTParam(wX,wY);
				const int nMaxDistance = 1+calcLightRadius(tParam);
				for (UINT y=wY-nMaxDistance; y<=wY+nMaxDistance; ++y)
					for (UINT x=wX-nMaxDistance; x<=wX+nMaxDistance; ++x)
						if (this->pRoom->IsValidColRow(x,y) && !clearedTiles.has(x,y))
						{
							const UINT wStartIndex = this->pRoom->ARRAYINDEX(x,y) * wLightValuesPerTile;
							memset(this->lightMaps.psDisplayedLight + wStartIndex, 0, wLightBytesPerTile);
							clearedTiles.insert(x,y);
						}
			}
		}
	}

	this->bRenderRoom = true;  //ready to refresh room image

	return true;
}

bool CRoomWidget::SetupDrawSquareInfo()
{
	const UINT wSquareCount = this->pRoom->CalcRoomArea();

	DeleteArrays();

	//Allocate new tile image arrays.
	this->pTileImages = new TileImages[wSquareCount];
	if (!this->pTileImages)
		return false;

	if (!this->lightMaps.init(wSquareCount * wLightValuesPerTile)) {
		DeleteArrays();
		return false;
	}

	//Init what might not be set below.
	ClearLights();
	for (UINT wIndex = 0; wIndex<wSquareCount; ++wIndex) {
		TileImages& ti = this->pTileImages[wIndex];
		ti.o = TI_FLOOR;
		ti.f = ti.tCovered = ti.t = T_EMPTY;
		ti.wallShadow = TI_UNSPECIFIED;

		//Give each m-layer tile a random animation frame
		ti.animFrame = RAND(6); //two or three possible frames for animated monsters
	}
	this->bAllDirty = true;

	return true;
}

void CRoomWidget::HighlightBombExplosion(const UINT x, const UINT y, const UINT tTile)
{
	CCueEvents CueEvents;
	CDbRoom room(*this->pRoom, false);
	room.InitRoomStats();
	CCoordStack bombs, powder_kegs;
	switch (tTile) {
	case T_BOMB: bombs.Push(x, y); break;
	case T_POWDER_KEG: powder_kegs.Push(x, y); break;
	default: break;
	}
	room.DoExplode(CueEvents, bombs, powder_kegs);

	CCoordSet coords;
	const CCoord* pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
		CueEvents.GetFirstPrivateData(CID_Explosion));
	while (pCoord)
	{
		coords.insert(pCoord->wX, pCoord->wY);
		pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
			CueEvents.GetNextPrivateData());
	}
	static const SURFACECOLOR ExpColor = { 224, 160, 0 };
	for (CCoordSet::const_iterator coord = coords.begin(); coord != coords.end(); ++coord)
		AddShadeEffect(coord->wX, coord->wY, ExpColor);
}

//*****************************************************************************
void CRoomWidget::TranslateMonsterColor(
	//Converts a single specially-formatted color value to rgb values
	//
	//Params:
	const int nColor,
	float& fR, float& fG, float& fB) //Out: rgb tinting values
{
	if (nColor < 0)
		fR = fG = fB = 1.0f;
	else {
		//nColor consists of three two-digit pairs: RRGGBB, where a value of "50" is normal.
		fR = (nColor % 1000000) / 500000.0f;
		fG = (nColor % 10000) / 5000.0f;
		fB = (nColor % 100) / 50.0f;
	}
}

//*****************************************************************************
void CRoomWidget::SetEffectsFrozen(const bool bIsFrozen)
// Updates frozen status of all effects layers. Frozen effects do not continue their animation and are suspended, waiting
{
	this->pLastLayerEffects->SetEffectsFrozen(bIsFrozen);
	this->pMLayerEffects->SetEffectsFrozen(bIsFrozen);
	this->pOLayerEffects->SetEffectsFrozen(bIsFrozen);
	this->pTLayerEffects->SetEffectsFrozen(bIsFrozen);
}

//*****************************************************************************
//Draws on each tile which directions are blocked on the selected pathmap (draws them as tiny squares)
void CRoomWidget::DebugDraw_MarkedTiles(SDL_Surface* pDestSurface) {
	static const SURFACECOLOR Color = { 128, 255, 255 };
	static const UINT CX_TILE = CBitmapManager::CX_TILE;
	static const UINT CY_TILE = CBitmapManager::CY_TILE;

	for (CCoordSet::const_iterator tile = CDbRoom::debugMarkedTiles.begin(); tile != CDbRoom::debugMarkedTiles.end(); ++tile) {
		this->ShadeRect(pDestSurface, Color, tile->wX, tile->wY, 1, 1);
		this->pTileImages[this->pRoom->ARRAYINDEX(tile->wX, tile->wY)].dirty = 1;
	}
}
//*****************************************************************************
//Draws on each tile which directions are blocked on the selected pathmap (draws them as tiny squares)
void CRoomWidget::DebugDraw_Pathmap(SDL_Surface* pDestSurface, MovementType eType) {
	static const SURFACECOLOR ColorWhite = { 255, 255, 255 };
	static const SURFACECOLOR ColorBlack = { 0, 0, 0 };
	static const UINT CX_TILE = CBitmapManager::CX_TILE;
	static const UINT CY_TILE = CBitmapManager::CY_TILE;
	static const UINT wNumNeighbors = 8;
	static const int dxDir[wNumNeighbors] = { -1,  0,  1, -1,  1, -1,  0,  1 };
	static const int dyDir[wNumNeighbors] = { -1, -1, -1,  0,  0,  1,  1,  1 };
	static const UINT rdirmask[] = { DMASK_SE, DMASK_S, DMASK_SW, DMASK_E, DMASK_W, DMASK_NE, DMASK_N, DMASK_NW };

	const CDbRoom* pRoom = this->GetRoom();
	const CPathMap* pPathmap = pRoom->pPathMap[eType];

	if (pPathmap == NULL)
		return;

	for (UINT wX = 0; wX < pRoom->wRoomCols; ++wX) {
		for (UINT wY = 0; wY < pRoom->wRoomRows; ++wY) {
			const SQUARE square = pPathmap->squares[pRoom->ARRAYINDEX(wX, wY)];

			for (UINT wO = 0; wO < wNumNeighbors; wO++) {
				if (square.eBlockedDirections & rdirmask[wO]) {
					SDL_Rect src = MAKE_SDL_RECT(
						this->GetX() + wX * CX_TILE + CX_TILE / 2 - 5 * dxDir[wO] - 3,
						this->GetY() + wY * CY_TILE + CY_TILE / 2 - 5 * dyDir[wO] - 3,
						4,
						4
					);
					this->DrawRect(src, ColorBlack, pDestSurface);
					src = MAKE_SDL_RECT(
						this->GetX() + wX * CX_TILE + CX_TILE / 2 - 5 * dxDir[wO] - 2,
						this->GetY() + wY * CY_TILE + CY_TILE / 2 - 5 * dyDir[wO] - 2,
						2,
						2
					);
					this->DrawRect(src, ColorWhite, pDestSurface);
				}
			}
		}
	}
}