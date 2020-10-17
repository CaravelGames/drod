// $Id: RoomWidget.h 9332 2008-10-31 00:53:10Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef ROOMWIDGET_H
#define ROOMWIDGET_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DrodBitmapManager.h"
#include "DrodEffect.h"
#include "FaceWidget.h"
#include "Light.h"
#include "TileImageCalcs.h"
#include "Scene.h"
#include <FrontEndLib/Widget.h>
#include <FrontEndLib/SubtitleEffect.h>
#include "../DRODLib/Swordsman.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/DbRooms.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Types.h>

//Range of weather parameters.
#define LIGHT_LEVELS (7)	//number of light levels
#define FOG_INCREMENTS (4)
#define SNOW_INCREMENTS (10)
#define RAIN_INCREMENTS (20)
extern const float fRoomLightLevel[LIGHT_LEVELS];
extern const float lightMap[3][NUM_LIGHT_TYPES];
extern const float darkMap[NUM_DARK_TYPES];

//Whether to draw edge on tile.
struct EDGES {
	EDGES() : north(EDGE_NONE), west(EDGE_NONE), south(EDGE_NONE), east(EDGE_NONE)
		, bHalfWall(false)
		, wPitX(0), wPitY(0), wPitRemaining(0) {}

	EDGETYPE north, west, south, east;

	bool     bHalfWall;	//draw inner wall texture on bottom-half of wall tile

	UINT wPitX, wPitY, wPitRemaining;	//pit edge rendering info
};

struct TileImages
{
	UINT o, f, t, /*tCovered,*/ wallShadow;
	EDGES edges; //black edges separating tile types
	vector<UINT> shadowMasks;

	//Info for repainting tiles.
	BYTE animFrame : 1;  //animation frame # of monster here
	BYTE damaged : 1;    //damaged tile must be updated on screen this frame
	BYTE dirty : 1;      //tile is dirty and needs to be repainted
	BYTE monster : 1;    //monster piece is on this tile
};

typedef USHORT LIGHTTYPE; //used to used a light value

struct LightMaps
{
	LightMaps()
		: psTempLightBuffer(NULL), psRoomLight(NULL)
		, psCeilingLight(NULL)
		, psPlayerLight(NULL), psDisplayedLight(NULL)
		, pActiveLight(NULL)
	{ }

	inline UINT bufferBytes() const { return bufferSize * sizeof(LIGHTTYPE); }

	void clear()
	{
		delete[] this->psTempLightBuffer;
		this->psTempLightBuffer = NULL;

		delete[] this->psRoomLight;
		this->psRoomLight = NULL;

		delete[] this->psCeilingLight;
		this->psCeilingLight = NULL;

		delete[] this->psPlayerLight;
		this->psPlayerLight = NULL;

		delete[] this->psDisplayedLight;
		this->psDisplayedLight = NULL;

		this->pActiveLight = NULL;
		this->bufferSize = 0;
	}

	void copy(LIGHTTYPE* dest, LIGHTTYPE* src) {
		memcpy(dest, src, bufferBytes());
	}

	bool init(UINT bytes)
	{
		bufferSize = bytes;

		ASSERT(!this->psDisplayedLight);

		this->psTempLightBuffer = new LIGHTTYPE[bufferSize];
		this->psRoomLight = new LIGHTTYPE[bufferSize];
		this->psCeilingLight = new LIGHTTYPE[bufferSize];
		this->psPlayerLight = new LIGHTTYPE[bufferSize];
		this->psDisplayedLight = new LIGHTTYPE[bufferSize];

		return this->psTempLightBuffer && this->psCeilingLight &&
			this->psRoomLight && this->psDisplayedLight && this->psPlayerLight;
	}

	void reset()
	{
		if (this->psRoomLight)
		{
			zero(this->psTempLightBuffer);
			zero(this->psRoomLight);
			zero(this->psCeilingLight);
			zero(this->psPlayerLight);
			zero(this->psDisplayedLight);
		}
	}

	void zero(LIGHTTYPE *buffer)
	{
		ASSERT(buffer);
		memset(buffer, 0, bufferBytes());
	}


	LIGHTTYPE *          psTempLightBuffer; //light buffer that may be used for the result
	                                        //of a functional manipulation on another light buffer
	LIGHTTYPE *          psRoomLight;     //RGB light layer for current room state
	LIGHTTYPE *          psCeilingLight;  //static RGB lighting from above
	LIGHTTYPE *          psPlayerLight;   //light from player
	LIGHTTYPE *          psDisplayedLight;//RGB lighting that is showed on screen
	LIGHTTYPE *          pActiveLight;    //tracks which light layer is being written to

	UINT bufferSize;
};

//******************************************************************************
class CCurrentGame;
class CRoomEffectList;
class CPlayerDouble;
class CCharacter;
class CCitizen;
class CSubtitleEffect;
class CFiredCharacterCommand;
class CRoomWidget : public CWidget
{
friend class CRoomEffectList; //to access dirtyTiles
friend class CGameScreen; //to call some protected rendering methods
friend class CDrodScreen; //to call some protected rendering methods
friend class CEditRoomScreen; //to call some protected rendering methods

public:
	CRoomWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW,
			UINT wSetH);

	void           AddColorToTile(SDL_Surface* pDestSurface, const int nAddColor, const UINT wTileImageNo,
			const UINT nPixelX, const UINT nPixelY, const UINT wWidth, const UINT wHeight, const int nXOffset=0, const int nYOffset=0);
	bool           AddDoorEffect(COrbAgentData *pOrbAgent);
	void           AddInfoSubtitle(CMoveCoord *pCoord, const WSTRING& wstr,
			const Uint32 dwDuration, const UINT displayLines=1, const SDL_Color& color=Black,
			const UINT fadeDuration=500);
	void           AddJitter(const CMoveCoord& coord, const float fDamagePercent);
	void           AddLastLayerEffect(CEffect *pEffect);
	void           AddMLayerEffect(CEffect *pEffect);
	void           AddOLayerEffect(CEffect *pEffect);
	void           AddPlayerLight(const bool bAlwaysRefresh=false);
	void           AddShadeEffect(const UINT wX, const UINT wY,
			const SURFACECOLOR &Color);
	void           AddStrikeOrbEffect(const COrbData &SetOrbData, bool bDrawOrb = true);
	CSubtitleEffect* AddSubtitle(CFiredCharacterCommand *pCommand, const Uint32 dwDuration);
	void           AddTLayerEffect(CEffect *pEffect);
	void           AddToSubtitles(CSubtitleEffect *pEffect);
	void				AddZombieGazeEffect(const CMonster *pZombie);
	void				AllowSleep(const bool bVal);
	void           AnimateMonster(CMonster* pMonster);
	bool           AreCheckpointsVisible() const {return this->bShowCheckpoints;}
	void           ClearEffects(const bool bKeepFrameRate = true);
	void           DirtyRoom() {this->bAllDirty = true;}
	virtual void   DisplayChatText(const WSTRING& text, const SDL_Color& color);
	void				DisplayRoomCoordSubtitle(const int nMouseX, const int nMouseY);
	void           DisplaySubtitle(const WCHAR *pText, const UINT wX, const UINT wY,
			const bool bReplace);
	void           DontAnimateMove();
	void           DrawPlatforms(SDL_Surface *pDestSurface, const bool bEditor=false, const bool bMoveInProgress=false);
	virtual void   DrawMonsters(CMonster *const pMonsterList,
			SDL_Surface *pDestSurface, const bool bActionIsFrozen,
			const bool bMoveInProgress=false);
	void				FadeToLightLevel(const UINT wNewLight);
	void           FinishMoveAnimation() {this->dwCurrentDuration = this->dwMoveDuration;}
	void           GetSquareRect(UINT wCol, UINT wRow, SDL_Rect &SquareRect) const;
	static UINT    GetKeyMID(const UINT param);
	static UINT    GetOrbMID(const UINT type);
	static UINT    GetPressurePlateMID(const UINT type);
	static UINT    GetSwordMID(const UINT param);
	static UINT    GetShieldMID(const UINT param);
	static UINT    GetAccessoryMID(const UINT param);
	UINT           GetTextureIndexForTile(const UINT tileNo, const bool bForceBaseImage) const;
	static UINT    GetTokenMID(const UINT param);
	UINT           GetCustomEntityTile(const UINT wLogicalIdentity,
			const UINT wO, const UINT wFrame) const;
	static UINT    GetCustomTileIndex(const UINT wO);
	UINT           GetEntityTile(const UINT wApparentIdentity,
			const UINT wLogicalIdentity, const UINT wO, const UINT wFrame) const;
	UINT           GetLastTurn() const {return this->wLastTurn;}
	WSTRING        GetMonsterInfo(const UINT wX, const UINT wY, const bool bFull) const;
	WSTRING        GetMonsterAbility(CMonster* pMonster) const;
	WSTRING        GetMonsterName(CMonster* pMonster) const;
//	UINT*          GetMonsterTile(const UINT wCol, const UINT wRow);
	UINT           GetMoveDuration() const {return this->dwMoveDuration;}
	CDbRoom*       GetRoom() const {return this->pRoom;}
	UINT           GetStockEntityTile(const UINT wIdentity, const UINT wO, const UINT wFrame) const;
	UINT           GetSwordTile(const UINT wType, const UINT wO, UINT sword=NoSword) const;
	UINT           GetTileForCustomChar(HoldCharacter *pCustomChar, const UINT wO, const UINT wFrame) const;
	void           HideCheckpoints() {this->bShowCheckpoints = false;}
	void           HidePlayer() {this->bShowingPlayer = false;}
	void           HighlightSelectedTile();
	virtual bool   IsDoubleClickable() const {return false;}
	bool           IsLightingRendered() const;
	bool           IsMonsterInvolvedInDeath(CMonster *pMonster) const;
	bool           IsMoveAnimating() const {return this->dwMovementStepsLeft != 0;}
	virtual bool   IsPlayerLightRendered() const;
	bool           IsPlayerLightShowing() const;
	bool           IsWeatherRendered() const;
	bool           LoadFromCurrentGame(CCurrentGame *pSetCurrentGame, const bool bLoad=true);
	bool           LoadFromRoom(CDbRoom *pRoom, const bool bLoad=true);
	void           LoadRoomImages();

	virtual void   HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);
	virtual void   Paint(bool bUpdateRect = true);
	virtual void   PaintClipped(const int nX, const int nY, const UINT wW,
			const UINT wH, const bool bUpdateRect = true);
	bool           PlayerLightTurnedOff() const;
	void           PutTLayerEffectsOnMLayer();

	void           RedrawMonsters(SDL_Surface* pDestSurface);
	void           RemoveLastLayerEffectsOfType(const EffectType eEffectType, const bool bForceClearAll=true);
	void           RemoveMLayerEffectsOfType(const EffectType eEffectType);
	void           RemoveOLayerEffectsOfType(const EffectType eEffectType);
	void           RemoveTLayerEffectsOfType(const EffectType eEffectType);
	void				RenderEnvironment(SDL_Surface *pDestSurface=NULL);
	void           RenderRoom(int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS, int wHeight=CDrodBitmapManager::DISPLAY_ROWS,
			const bool bEditor=true);
	void           RenderRoomInPlay(int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS, int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           RerenderRoom() {this->bRenderRoom = true; DirtyRoom(); }
	void           RenderRoomLighting() {this->bRenderRoomLight = true;}
	void           RenderRoomTileObjects(const UINT wX, const UINT wY,
			const int nX, const int nY, SDL_Surface *pDestSurface,
			const UINT wOTileNo, const TileImages& ti, LIGHTTYPE *psL,
			const float fDark, const bool bAddLight, const bool bAddLightLayers,
			const bool bEditor, const bool bPitPlatformTiles=false);
	void           ResetForPaint();
	void           ResetJitter();
	void           ResetRoom() {this->pRoom = NULL;}
	void           SetAnimateMoves(const bool bAnimate) {this->bAnimateMoves = bAnimate;}
	void           SetMoveDuration(const UINT dwDuration) {this->dwMoveDuration = dwDuration;}
	void           SetOpacityForMLayerEffectsOfType(const EffectType eEffectType, float fOpacity);
	virtual void   SetPlot(const UINT /*wCol*/, const UINT /*wRow*/) {}
	void           ShowCheckpoints(const bool bVal=true) {this->bShowCheckpoints = bVal;}
	void           ShowRoomTransition(const UINT wExitOrientation, const bool bShowPlayer = false);
	void           ShowPlayer(const bool bFlag=true) {this->bShowingPlayer = bFlag;}
	void           StopSleeping();
	bool           SubtitlesHas(CSubtitleEffect *pEffect) const;
	UINT           AdvanceAnimationFrame(const UINT wCol, const UINT wRow);
	void           ToggleFrameRate();
	void           ToggleMoveCount();
	void           ToggleVarDisplay();
	static void    TranslateMonsterColor(const int nColor, float& fR, float& fG, float& fB);
	void           UnloadCurrentGame();
	void           UpdateFromCurrentGame(const bool bForceReload=false);
	void           UpdateFromPlots(const CCoordSet *pSet, CCoordSet *pGeometryChanges);

	const CCurrentGame* GetCurrentGame() const {return this->pCurrentGame;}

	SDL_Surface*  pRoomSnapshotSurface;   //image of the pre-rendered room

	//Room light level.
	UINT              wDark;		      //level of room darkening to apply (0 = off)
	bool              bCeilingLightsRendered; //reset to re-render ceiling lights

protected:
	virtual  ~CRoomWidget();

	virtual void   HandleAnimate() {if (this->pRoom) Paint(true);}
	virtual bool   IsAnimated() const {return this->bAnimateMoves;}
	virtual bool   Load();
	virtual void   Unload();

	void           AddJitterOffset(const UINT wX, const UINT wY,
			UINT& wXOffset, UINT& wYOffset);
	void           AddObstacleShadowMask(const UINT wCol, const UINT wRow);
	void           AddLight(SDL_Surface *pDestSurface, const UINT wX, const UINT wY,
			LIGHTTYPE* sRGBIntensity, const float fDark, const UINT wTileMask,
			const Uint8 opacity=255) const;
	void           AddLightInterp(SDL_Surface *pDestSurface, const UINT wX, const UINT wY,
			LIGHTTYPE* sRGBIntensity, const float fDark,
			const UINT wTileMask=TI_UNSPECIFIED, const Uint8 opacity=255,
			const int yRaised=0) const;
	void           AddLightOffset(SDL_Surface *pDestSurface,
			const UINT wCol, const UINT wRow, const UINT wXOffset, const UINT wYOffset,
			const UINT wTileImageNo, const bool bDrawRaised, const Uint8 nOpacity) const;
	virtual void   AnimateMonsters();
//	void           BetterVisionQuery();
	void           BoundsCheckRect(int &wCol, int &wRow,
			int &wWidth, int &wHeight) const;
	void           BAndWRect(SDL_Surface *pDestSurface,
			int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS,
			int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           BlitDirtyRoomTiles(const bool bMoveMade);
	void           BlitTileShadowsOnMovingSprite(const UINT wCol, const UINT wRow,
			const UINT wXOffset, const UINT wYOffset, const UINT wTileImageNo,
			SDL_Surface *pDestSurface);
	void           CastLightOnTile(const UINT wX, const UINT wY,
			const PointLightObject& light, const bool bGeometry=true);
	void           ClearLights();
	void           DarkenRect(SDL_Surface *pDestSurface,
			const float fLightPercent, int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS,
			int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void				DisplayRoomCoordSubtitle(const UINT wX, const UINT wY);
	void           DrawDoorFiller(SDL_Surface *pDestSurface, const UINT wX, const UINT wY);
	void           ShadeRect(SDL_Surface *pDestSurface,
			const SURFACECOLOR &Color, int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS,
			int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           DeleteArrays();
	void           DirtyTileRect(const int x1, const int y1,
			const int x2, const int y2);
	void           DrawBoltInRoom(const int xS, const int yS, const int xC,
			const int yC);
	void           DrawDamagedMonsters(SDL_Surface *pDestSurface);
	void           DrawDamagedMonsterSwords(SDL_Surface *pDestSurface);
	void           DrawInvisibilityRange(const int nX, const int nY,
			SDL_Surface *pDestSurface, CCoordIndex *pCoordIndex=NULL, const int nRange=DEFAULT_SMELL_RANGE);
	virtual void   DrawCharacter(CCharacter *pCharacter, const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	virtual bool   DrawingSwordFor(const CMonster* /*pMonster*/) const { return true; }
//	void           DrawCitizen(CCitizen *pCitizen, const bool bDrawRaised,
//			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawDouble(const CPlayerDouble *pDouble, const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress, const Uint8 nOpacity=255);
//	void           DrawDoubleCursor(const UINT wCol, const UINT wRow, SDL_Surface *pDestSurface);
	void           DrawMonster(CMonster *const pMonster, CDbRoom *const pRoom,
			SDL_Surface *pDestSurface, const bool bActionIsFrozen,
			const bool bMoveInProgress=true, const bool bDrawPieces=true);
	void           DrawMonsterKillingPlayer(SDL_Surface *pDestSurface);
	void           DrawPlayer(const CSwordsman &swordsman, SDL_Surface *pDestSurface);
	bool           DrawRaised(const UINT wTileNo) const {return bIsElevatedTile(wTileNo);}
	void           DrawRockGiant(const CMonster *pMonster,	const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawSerpent(CMonster *pMonster, SDL_Surface *pDestSurface, const bool bMoveInProgress);
//	void           DrawSlayerWisp(const CPlayerDouble *pDouble, SDL_Surface *pDestSurface);

	void           DrawSwordFor(const CMonster *pMonster, const UINT wType,
			const UINT wMSwordX, const UINT wMSwordY, const UINT wXOffset, const UINT wYOffset,
			const bool bDrawRaised, SDL_Surface *pDestSurface, const bool bMoveInProgress,
			const Uint8 nOpacity=255, const UINT nColor=0);
	void           DrawSwordFor(const CMonster *pMonster, const UINT wMSwordX,
			const UINT wMSwordY, const UINT wXOffset, const UINT wYOffset,
			const bool bDrawRaised, const UINT wSwordTI, SDL_Surface *pDestSurface,
			const bool bMoveInProgress, const Uint8 nOpacity=255, const UINT nColor=0);
	void           DrawSwordsFor(const vector<CMonster*>& drawnMonsters, SDL_Surface *pDestSurface);
	UINT           GetSwordTileFor(const CMonster* pMonster, const UINT wO, const UINT wType) const;
	UINT           GetSwordTileFor(const UINT wMonsterType, const UINT wO, const UINT wWeaponType) const;

	void           DrawTileEdges(const UINT wX, const UINT wY,
			const TileImages* pTI, SDL_Surface *pDestSurface);
	void           DrawTileImage(const UINT wCol, const UINT wRow,
			const UINT wXOffset, const UINT wYOffset,
			const UINT wTileImageNo, const bool bDrawRaised, SDL_Surface *pDestSurface,
			const bool bDirtyTiles, const Uint8 nOpacity=255, bool bClipped=false, const int nAddColor=-1);
	CEntity*       GetLightholder() const;
	bool           GetPlayerDisplayTiles(const CSwordsman &swordsman,
			UINT& wO, UINT& wFrame, UINT& wSManTI, UINT& wSwordTI) const;
	float          getTileElev(const UINT i, const UINT j) const;
	float          getTileElev(const UINT wOTile) const;
	void           GetWeather();
	void           JitterBoundsCheck(const UINT wX, const UINT wY,
			UINT& wXOffset, UINT& wYOffset);
	void           LowPassLightFilter(LIGHTTYPE *pSrc, LIGHTTYPE *pDest,
			const bool bLeft, const bool bRight, const bool bUp, const bool bDown) const;
	void           modelVertTileface(const float elev, const UINT i, const UINT j,
			const bool bXAxis, const bool bNorthernWall);
	void           ProcessLightmap();
	void           ReduceJitter();
	void           RemoveHighlight();
	void           RenderFogInPit(SDL_Surface *pDestSurface=NULL);
	void           RenderRoomItemsOnTiles(const CCoordIndex& tiles, SDL_Surface *pDestSurface,
			const float fLightLevel, const bool bAddLight, const bool bEditor);
	void           RenderRoomModel(const int nX1, const int nY1, const int nX2, const int nY2);
	void           RenderRoomLayers(SDL_Surface* pSurface, const bool bDrawPlayer=true);
	virtual bool   SkyWillShow() const;
	void           SetCeilingLight(const UINT wX, const UINT wY);
	void           SetFrameVars(const bool bMoveMade);
	void           SetMoveCountText();
	bool           SetupDrawSquareInfo();
	void           ShowFrameRate(const bool bVal);
	void           ShowMoveCount(const bool bVal);
	void           ShowVarUpdates(const bool bVal);
	void           SynchRoomToCurrentGame();
	void           SyncRoomPointerToGame(CCurrentGame* pGame);
	bool           UpdateDrawSquareInfo(const CCoordSet *pSet=NULL, CCoordSet *pGeometryChanges=NULL);
	void           UpdateRoomRects();

	UINT             dwRoomX, dwRoomY;
	WSTRING           style;
	UINT              wShowCol, wShowRow;

	CCurrentGame *       pCurrentGame;  //to show room of a game in progress
	CDbRoom *            pRoom;         //to show room in initial state
	TileImages *         pTileImages;   //layer tile images
	bool                 bLastVision;   //room vision type

	Scene                model;           //model of the room

	LightMaps            lightMaps;
	CCoordSet            lightedPlayerTiles; //tiles that have player's light cast onto them
	CCoordSet            lightedRoomTiles;   //room tiles that have light cast onto them
	CCoordSet           *pActiveLightedTiles;//tiles being marked with light
	CCoordSet            partialLightedTiles;//tiles with some light and shadow on them
	CCoordIndex          tileLightInfo;      //property of lighting applied to each tile
	CCoordIndex          jitterInfo;         //how much jitter applied to the sprite on each tile
	bool                 bRenderRoom;        //flag indicates room must be re-rendered
	bool                 bRenderRoomLight;   //flag indicates room lighting must be re-rendered
	bool                 bRenderPlayerLight; //flag indicates player lighting must be re-rendered
	UINT                 wLastPlayerLightX, wLastPlayerLightY; //where player light was rendered last frame
	pair<float, float>   cursorLight;     //player may place a light in the room to see better

	CRoomEffectList *       pLastLayerEffects;
	CRoomEffectList *       pMLayerEffects;
	CRoomEffectList *       pOLayerEffects;
	CRoomEffectList *       pTLayerEffects;
	SUBTITLES         subtitles;

	bool              bShowingPlayer;   //whether player is visible onscreen
	bool              bShowCheckpoints;
	bool              bShowFrameRate, bShowMoveCount, bShowVarUpdates;
	bool              bAddNEffect;   //for 'Neather striking orb
//	bool              bRequestEvilEyeGaze; //for vision power-up
	Uint8             ghostOpacity;
	UINT              wHighlightX, wHighlightY; //user highlight position
	CCoordSet         movingTLayerObjectsToRender;

	UINT              dwLastDrawSquareInfoUpdateCount;
	Uint32            dwLastAnimationFrame;   //monster animation

	//For animating a turn
	UINT              dwMovementStepsLeft;
	Uint32            dwMoveDuration;   //duration of one movement
	Uint32            dwCurrentDuration;//total duration of all frames this turn
	Uint32				dwLastFrame, dwTimeSinceLastFrame; //time last frame was drawn (ms)
	bool              bFinishLastMovementNow;
	bool              bAnimateMoves, bAnimationInProgress;
	bool					bAllowSleep;		//whether hero can go to sleep (fidget)
	//bool              bNextWispFrame;
	bool              bPlayerSleeping;  //fidget
	bool              bJitterThisFrame;      //jitter updating
	Uint32            dwLastJitterReduction;

	//Environmental effects.
	bool					bOutside, bSkyVisible; //outside, sky/ceiling is visible
	UINT					dwSkyX;           //sky image offset
	bool					bLightning;       //lightning flash
	Uint32				dwLightning;      //time flash started
	bool					bFog;             //rolling fog
	float					fFogX, fFogY;     //fog mask offset
	float					fFogOldX, fFogOldY; //fog mask offset at last redraw
	float					fFogVX, fFogVY;   //fog "wind" velocity
	BYTE					cFogLayer;        //how high fog extends
	bool					bClouds;          //clouds
	float					fCloudX, fCloudY;	//cloud mask offset
	float					fCloudOldX, fCloudOldY;	//cloud mask offset at last redraw
	float					fCloudAngle;      //direction of wind
	bool              bSunlight;        //shining through clouds onto ground
	UINT              wSnow;            //snowflakes are falling (0 = off)
	bool              bSkipLightfade;   //whether light crossfade is skipped
	WSTRING           sky;              //non-default sky image
	queue<UINT>       playThunder;      //when to play a thunder sound

	SDL_Surface      *pSkyImage;        //sky image
	WSTRING           wstrSkyImage;     //name of sky image

	//vars for optimization in rendering the room
	bool              bAllDirty;  //all room tiles must be redrawn
//	bool              bWasPlacingDouble;   //player was placing double last frame
//	bool              bWasInvisible;    //swordsman was invisible last frame
	UINT              wLastTurn;  //turn # at last frame
	UINT              wLastOrientation; //direction swords were pointing last turn
	UINT              wLastX, wLastY;   //position of player last turn
	bool              bLastRaised;      //was player raised last turn

	int               CX_TILE, CY_TILE;

private:
	bool           NeedsSwordRedrawing(const CMonster *pMonster) const;

	void           PropagateLight(const float fSX, const float fSY, const UINT tParam, const bool bCenterOnTile=true);			
	void           PropagateLightNoModel(const int nSX, const int nSY, const UINT tParam);

	void           PlacePlayerLightAt(int pixel_x, int pixel_y);
	void           PropagatePlayerLight();
	void           RenderPlayerLight();
	void           ResetPlayerLightMap();
	void           ResetUserLightsource();
	bool           ShowShadowCastingAnimation() const;

	void           flag_weather_refresh();
	void           SetFrameVarsForWeather();

	Uint32         time_of_last_weather_render;
	int            redrawingRowForWeather;
	bool           need_to_update_room_weather;

	Uint32         time_of_last_sky_move;
};

#endif //#ifndef ROOMWIDGET_H
