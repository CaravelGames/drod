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

#ifndef ROOMWIDGET_H
#define ROOMWIDGET_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DrodBitmapManager.h"
#include "DrodEffect.h"
#include "FaceWidget.h"
#include "TileImageCalcs.h"
#include "Light.h"
#include "Scene.h"
#include "PuzzleModeOptions.h"
#include <FrontEndLib/Widget.h>
#include <FrontEndLib/SubtitleEffect.h>
#include "NoticeEffect.h"
#include "../DRODLib/CueEvents.h"
#include "../DRODLib/Swordsman.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/DbRooms.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Types.h>

//Range of weather parameters.
#define LIGHT_LEVELS (7) //number of light levels
#define FOG_INCREMENTS (4)
#define SNOW_INCREMENTS (10)
#define RAIN_INCREMENTS (20)
extern const float fRoomLightLevel[LIGHT_LEVELS];
extern const float lightMap[3][NUM_LIGHT_TYPES];
extern const float darkMap[NUM_DARK_TYPES];

struct MonsterAnimation {
	UINT currentFrame;
	UINT frameTimerPosition;
	UINT frameDuration;
	UINT waitUntilPause;
	UINT pauseTimer;
	MonsterAnimation() {}
	MonsterAnimation(UINT cf, UINT ftp, UINT fd, UINT wup, UINT pt)
		: currentFrame(cf), frameTimerPosition(ftp), frameDuration(fd),
		waitUntilPause(wup), pauseTimer(pt)
	{ }
};

//Whether to draw edge on tile.
struct EDGES {
	EDGES() : north(EDGE_NONE), west(EDGE_NONE), south(EDGE_NONE), east(EDGE_NONE)
		, bHalfWall(false)
		, wPitX(0), wPitY(0), wPitRemaining(0) {}

	EDGETYPE north, west, south, east;

	bool     bHalfWall;	//draw inner wall texture on bottom-half of wall tile

	UINT wPitX, wPitY, wPitRemaining;	//pit edge rendering info
};

typedef USHORT LIGHTTYPE; //represents a light value on one color channel

struct TileImages
{
	UINT o, f, t, tCovered, wallShadow;
	EDGES edges; //black edges separating tile types
	vector<UINT> shadowMasks;

	//Info for repainting tiles.
	BYTE animFrame;  //animation frame # of monster here
	BYTE damaged : 1;    //damaged tile must be updated on screen this frame
	BYTE dirty : 1;      //tile is dirty and needs to be repainted
	BYTE monster : 1;    //monster piece is on this tile
};

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

typedef map<ROOMCOORD, vector<TweeningTileMask> > t_PitMasks;

//******************************************************************************
struct TileImageBlitParams {
	TileImageBlitParams(UINT col, UINT row, UINT tile, UINT xOffset=0, UINT yOffset=0, bool dirtyTiles=false, bool drawRaised=false)
		: wCol(col), wRow(row)
		, wTileImageNo(tile)
		, wXOffset(xOffset), wYOffset(yOffset)
		, bDirtyTiles(dirtyTiles)
		, bDrawRaised(drawRaised)
		, nOpacity(255)
		, bClipped(false)
		, nAddColor(-1)
		, bCastShadowsOnTop(true)
		, appliedDarkness(0.75) //reduce overhead darkness applied to entities to make them a bit more visible
		, nCustomColor(-1)
	{ }
	TileImageBlitParams(const TileImageBlitParams& rhs);

	static inline bool CropRectToTileDisplayArea(SDL_Rect& BlitRect);
	static void resetDisplayArea() { crop_display = false; }
	static void setDisplayArea(int x, int y, int w, int h);
	static bool crop_display;
	static SDL_Rect display_area;

	UINT wCol, wRow;
	UINT wTileImageNo;
	UINT wXOffset, wYOffset;
	bool bDirtyTiles;
	bool bDrawRaised;
	Uint8 nOpacity;
	bool bClipped;
	int nAddColor;
	int nCustomColor;
	bool bCastShadowsOnTop;
	float appliedDarkness; // Normally monsters are drawn with 75% ceiling darkness, but moving T-Objects need to be drawn with the
	                       // same opacity as the stationary ones, which is 100% ceiling darkness, otherwise things look weird.
};

//******************************************************************************
class CCurrentGame;
class CRoomEffectList;
class CArmedMonster;
class CCharacter;
class CCitizen;
class CSubtitleEffect;
class CTemporalClone;
class CFiredCharacterCommand;
class CRoomWidget : public CWidget
{
friend class CRoomEffectList; //to access dirtyTiles
friend class CGameScreen; //to call some protected rendering methods
friend class CDrodScreen; //" "
friend class CRoomScreen; //for notices
friend class CEditRoomScreen; //to call some protected rendering methods

public:
	CRoomWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW,
			UINT wSetH);

	bool           AddDoorEffect(COrbAgentData *pOrbAgent);
	void           AddInfoSubtitle(CMoveCoord *pCoord, const WSTRING& wstr,
			const Uint32 dwDuration, const UINT displayLines=1, const SDL_Color& color=Black,
			const UINT fadeDuration=500);
	void           AddJitter(const CMoveCoord& coord, const float fPercent);
	void           AddLayerEffect(CEffect *pEffect, int layer);
	void           AddLastLayerEffect(CEffect *pEffect);
	void           AddMLayerEffect(CEffect *pEffect);
	void           AddOLayerEffect(CEffect *pEffect);
	bool           AddOrbEffect(COrbAgentData *pOrbAgent);
	void           AddPlayerLight(const bool bAlwaysRefresh=false);
	void           AddShadeEffect(const UINT wX, const UINT wY,
			const SURFACECOLOR &Color);
	void           AddStrikeOrbEffect(const COrbData &SetOrbData, bool bDrawOrb = true);
	CSubtitleEffect* AddSubtitle(CFiredCharacterCommand *pCommand, const Uint32 dwDuration);
	void           AddTLayerEffect(CEffect *pEffect);
	void           AddToSubtitles(CSubtitleEffect *pEffect);
	void           AddAumtlichGazeEffect(const CMonster *pAumtlich);
	UINT           AdvanceAnimationFrame(const UINT wCol, const UINT wRow);
	void           AdvanceAnimationFrame(const CMonster *pMonster);
	void           AllowSleep(const bool bVal);
	bool           AreCheckpointsVisible() const {return this->bShowCheckpoints;}
	void           ClearEffects(const bool bKeepFrameRate = true);
	void           CountDirtyTiles(UINT& damaged, UINT& dirty, UINT& monster) const;
	void           DirtyRoom() {this->bAllDirty = true;}
	void           DisplayPersistingImageOverlays(CCueEvents& CueEvents);
	void           DisplayRoomCoordSubtitle(const int nMouseX, const int nMouseY);
	void           DisplaySubtitle(const WCHAR *pText, const UINT wX, const UINT wY,
			const bool bReplace);
	void           DontAnimateMove();
	virtual void   DrawMonsters(CMonster *const pMonsterList,
			SDL_Surface *pDestSurface, const bool bActionIsFrozen,
			const bool bMoveInProgress=false);
	void           FadeToLightLevel(const UINT wNewLight, CCueEvents& CueEvents);
	void           FinishMoveAnimation() {this->dwCurrentDuration = this->dwMoveDuration;}
	void           GetSquareRect(UINT wCol, UINT wRow, SDL_Rect &SquareRect) const;
	UINT           GetTextureIndexForTile(const UINT tileNo, const bool bForceBaseImage) const;
	static UINT    GetOrbMID(const UINT type);
	static UINT    GetPressurePlateMID(const UINT type);
	static UINT    GetTokenMID(const UINT param);
	virtual UINT   GetCustomEntityTile(const UINT wLogicalIdentity, const UINT wO, const UINT wFrame) const;
	UINT           GetCustomEntityTileFor(const HoldCharacter *pCustomChar, const UINT wO, const UINT wFrame) const;
	static UINT    GetCustomTileIndex(const UINT wO);
	UINT           GetEntityTile(const UINT wApparentIdentity,
			const UINT wLogicalIdentity, const UINT wO, const UINT wFrame) const;
	UINT           GetLastTurn() const {return this->wLastTurn;}
	UINT           GetMoveDuration() const {return this->dwMoveDuration;}
	CDbRoom*       GetRoom() const {return this->pRoom;}
	UINT           GetStockEntityTile(const UINT wIdentity, const UINT wO, const UINT wFrame) const;

	void           HideCheckpoints() {this->bShowCheckpoints = false;}
	void           HidePlayer() {this->bShowingPlayer = false;}
	void           HighlightSelectedTile();
	virtual bool   IsDoubleClickable() const {return false;}
	bool           IsLightingRendered() const;
	bool           IsMonsterInvolvedInDeath(CMonster *pMonster) const;
	bool           IsMoveAnimating() const {return this->dwMovementStepsLeft != 0;}
	virtual bool   IsPlayerLightRendered() const;
	bool           IsPlayerLightShowing() const;
	bool           IsShowingMoveCount() const {return this->bShowMoveCount;}
	bool           IsShowingPuzzleMode() const {return this->bShowPuzzleMode;}
	bool           IsTemporalCloneAnimated() const { return this->temporalCloneEffectHeight >= 0; }
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
	void           ProcessCueEventsBeforeRoomDraw(const CCueEvents &CueEvents);

	void           QueueRemoveLayerEffectsOfType(const EffectType eEffectType, int layer) {
		queued_layer_effect_type_removal.insert(make_pair(eEffectType, layer)); }

	void           RedrawMonsters(SDL_Surface* pDestSurface);
	void           RemoveGroupEffects(int clearGroup);
	void           RemoveLayerEffects(const EffectType eEffectType, int layer);
	void           RemoveLastLayerEffectsOfType(const EffectType eEffectType, const bool bForceClearAll=true);
	void           RemoveMLayerEffectsOfType(const EffectType eEffectType);
	void           RemoveOLayerEffectsOfType(const EffectType eEffectType);
	void           RemoveTLayerEffectsOfType(const EffectType eEffectType);
	void	       RenderEnvironment(SDL_Surface *pDestSurface=NULL);
	void           RenderRoom(int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS, int wHeight=CDrodBitmapManager::DISPLAY_ROWS,
			const bool bEditor=true);
	void           RenderRoomInPlay(int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS, int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           RenderRoomLayers(SDL_Surface* pSurface, const bool bDrawPlayer=true);
	void           RerenderRoom() {this->bRenderRoom = true; DirtyRoom(); }
	void           RenderRoomLighting() {this->bRenderRoomLight = true;}
	void           DrawTLayerTile(const UINT wX, const UINT wY,
			const int nX, const int nY, SDL_Surface *pDestSurface,
			const UINT wOTileNo, const TileImages& ti, LIGHTTYPE *psL,
			const float fDark, const bool bAddLight,
			const bool bEditor, const vector<TweeningTileMask>* pPitPlatformMasks = NULL);
	void           ResetForPaint();
	void           ResetJitter();
	void           ResetRoom() {this->pRoom = NULL;}
	void           SetAnimateMoves(const bool bAnimate) {this->bAnimateMoves = bAnimate;}
	void           SetDeathFadeOpacity(const float opacity) { this->fDeathFadeOpacity = opacity;  }
	void           SetEffectsFrozen(const bool bIsFrozen);
	void           SetMoveDuration(const UINT dwDuration) {this->dwMoveDuration = dwDuration;}
	void           SetOpacityForMLayerEffectsOfType(const EffectType eEffectType, float fOpacity);
	void           SetOpacityForEffectsOfType(const EffectType eEffectType, float fOpacity);
	virtual void   SetPlot(const UINT /*wCol*/, const UINT /*wRow*/) {}
	void           SetPuzzleModeOptions(const PuzzleModeOptions &puzzleModeOptions);
	void           ShowCheckpoints(const bool bVal=true) {this->bShowCheckpoints = bVal;}
	void           ShowRoomTransition(const UINT wExitOrientation, CCueEvents& CueEvents);
	void           ShowPlayer(const bool bFlag=true) {this->bShowingPlayer = bFlag;}
	void           ShowPuzzleMode(const bool bVal);
	void           ShowVarUpdates(const bool bVal);
	void           StopSleeping();
	bool           SubtitlesHas(CSubtitleEffect *pEffect) const;
	UINT           SwitchAnimationFrame(const UINT wCol, const UINT wRow);
	void           ToggleFrameRate();
	void           ToggleMoveCount();
	void           TogglePuzzleMode();
	void           ToggleVarDisplay();
	static void    TranslateMonsterColor(const int nColor, float& fR, float& fG, float& fB);
	void           UnloadCurrentGame();
	void           UpdateFromCurrentGame(const bool bForceReload=false);
	void           UpdateFromPlots(const CCoordSet *pSet, const CCoordSet *pGeometryChanges);

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
			const Uint8 opacity=255, SDL_Rect* crop=NULL) const;
	void           AddLightOffset(SDL_Surface *pDestSurface, const TileImageBlitParams& blit) const;
	void           AddLightInterp(SDL_Surface *pDestSurface, const UINT wX, const UINT wY,
			LIGHTTYPE* sRGBIntensity, const float fDark,
			const UINT wTileMask=TI_UNSPECIFIED, const Uint8 opacity=255,
			const int yRaised=0, SDL_Rect* crop=NULL) const;
	void           AnimateMonsters();
	void           BetterVisionQuery();
	void           BoundsCheckRect(int &wCol, int &wRow,
			int &wWidth, int &wHeight) const;
	void           BAndWRect(SDL_Surface *pDestSurface, int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS, int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           BandWTile(SDL_Surface *pDestSurface, int wCol, int wRow);
	void           BlitDirtyRoomTiles(const bool bMoveMade);
	void           CastLightOnTile(const UINT wX, const UINT wY, const float fLightSourceZTileElev,
			const PointLightObject& light, const bool bGeometry=true);
	void           ClearLights();
	void           DarkenRect(SDL_Surface *pDestSurface,
			const float fLightPercent, int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS,
			int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	virtual void   DisplayChatText(const WSTRING& text, const SDL_Color& color);
	void           DisplayRoomCoordSubtitle(const UINT wX, const UINT wY);
	void           ShadeRect(SDL_Surface *pDestSurface,
			const SURFACECOLOR &Color, int wCol=0, int wRow=0,
			int wWidth=CDrodBitmapManager::DISPLAY_COLS,
			int wHeight=CDrodBitmapManager::DISPLAY_ROWS);
	void           DeleteArrays();
	void           DirtyTileRect(const int x1, const int y1,
			const int x2, const int y2);
	void           DirtyTilesForSpriteAt(UINT pixel_x, UINT pixel_y, UINT w, UINT h);
	void           DrawBoltInRoom(const int xS, const int yS, const int xC,
			const int yC);
	void           DrawDamagedMonsters(SDL_Surface *pDestSurface, const CharacterDisplayMode eDisplayMode);
	void           DrawDamagedMonsterSwords(SDL_Surface *pDestSurface);
	void           DrawInvisibilityRange(const int nX, const int nY,
			SDL_Surface *pDestSurface, CCoordIndex *pCoordIndex=NULL, const int nRange=DEFAULT_SMELL_RANGE);
	virtual void   DrawCharacter(const CCharacter *pCharacter, bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	virtual bool   DrawingSwordFor(const CMonster* /*pMonster*/) const { return true; }
	void           DrawCitizen(const CCitizen *pCitizen, const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawArmedMonster(const CArmedMonster *pArmedMonster, const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress, const Uint8 nOpacity=255);
	void           DrawDoubleCursor(const UINT wCol, const UINT wRow,
			SDL_Surface *pDestSurface);
	void           DrawGentryiiChain(const CMonster *pMonster, SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawGentryiiChainLink(MonsterPieces::const_iterator pieceIt,
			MonsterPieces::const_iterator pieces_end,
			UINT wPrevX, UINT wPrevY,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawMonster(const CMonster* pMonster, CDbRoom *const pRoom,
			SDL_Surface *pDestSurface, const bool bActionIsFrozen,
			const bool bMoveInProgress=true, const bool bDrawPieces=true);
	void           DrawMonsterKillingPlayer(SDL_Surface *pDestSurface);
	void           DrawMonsterKilling(CCoord* pCoord, SDL_Surface *pDestSurface);
	void           DrawMonsterKillingAt(CCoord* pCoord, SDL_Surface *pDestSurface);
	void           DrawOverheadLayer(SDL_Surface *pDestSurface);
	void           DrawPlatformsAndTLayer(SDL_Surface *pDestSurface,
			const bool bEditor=false, const bool bMoveInProgress=false);
	void           DrawPlayer(const CSwordsman &swordsman,
			SDL_Surface *pDestSurface);
	bool           DrawRaised(const UINT wTileNo) const {
		return bIsDoor(wTileNo) || bIsWall(wTileNo) || bIsCrumblyWall(wTileNo);}
	void           DrawRockGiant(const CMonster *pMonster,	const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawSerpentBody(const CMonster *pMonster, SDL_Surface *pDestSurface, const bool bMoveInProgress);
	void           DrawSlayerWisp(const CMonster *pMonster, SDL_Surface *pDestSurface);
	void           DrawSwordFor(const CMonster *pMonster, const UINT wType,
			TileImageBlitParams& blit, SDL_Surface *pDestSurface);
	void           DrawSwordFor(const CMonster *pMonster, TileImageBlitParams& blit, SDL_Surface *pDestSurface);
	void           DrawSwordsFor(const vector<CMonster*>& drawnMonsters, SDL_Surface *pDestSurface);

	void           DrawDoorFiller(SDL_Surface *pDestSurface, const UINT wX, const UINT wY);
	void     DrawTileEdges(const UINT wX, const UINT wY,
			const TileImages* pTI, SDL_Surface *pDestSurface);

	void           DrawTileImage(const TileImageBlitParams& blit, SDL_Surface* pDestSurface);
	void           DrawTileImageWithoutLight(const TileImageBlitParams& blit, SDL_Surface* pDestSurface);
	void           DrawTileLight(const TileImageBlitParams& blit, SDL_Surface* pDestSurface);
	bool           ClipTileArea(int nPixelX, int nPixelY, SDL_Rect& BlitRect) const;

	CEntity*       GetLightholder() const;
	UINT           GetSwordTileFor(const CMonster *pMonster, const UINT wO, const UINT wType) const;
	UINT           GetSwordTileFor(const UINT wMonsterType, const UINT wO, const UINT wWeaponType) const;
	float          getTileElev(const UINT i, const UINT j) const;
	float          getTileElev(const UINT wOTile) const;
	void           GetWeather();
	void           HighlightBombExplosion(const UINT x, const UINT y, const UINT tTile);
	void           JitterBoundsCheck(const UINT wX, const UINT wY,
			UINT& wXOffset, UINT& wYOffset);
	void           LowPassLightFilter(LIGHTTYPE *pSrc, LIGHTTYPE *pDest,
			const bool bLeft, const bool bRight, const bool bUp, const bool bDown) const;
	void           modelVertTileface(const float elev, const UINT i, const UINT j,
			const bool bXAxis, const bool bNorthernWall);
	void           NegativeTile(SDL_Surface *pDestSurface, int wCol, int wRow);
	void           ProcessLightmap();
	void           ReduceJitter();
	void           RemoveHighlight();
	void           RenderFogInPit(SDL_Surface *pDestSurface=NULL);
	void           RenderRoomModel(const int nX1, const int nY1, const int nX2, const int nY2);
	void           DrawTLayerTiles(const CCoordIndex& tiles, const t_PitMasks& pitMasks,
			SDL_Surface *pDestSurface,
			const float fLightLevel, const bool bAddLight, const bool bEditor);
	void           SepiaTile(SDL_Surface *pDestSurface, int wCol, int wRow);
	virtual bool	SkyWillShow() const;
	void           SetCeilingLight(const UINT wX, const UINT wY);
	void           SetFrameVars(const bool bMoveMade);
	void           SetMoveCountText();
	bool           SetupDrawSquareInfo();
	void           ShowFrameRate(const bool bVal);
	void           ShowMoveCount(const bool bVal);
	void           SynchRoomToCurrentGame();
	void           SyncRoomPointerToGame(CCurrentGame* pGame);
	bool           UpdateDrawSquareInfo(const CCoordSet *pSet=NULL, const CCoordSet *pGeometryChanges=NULL);
	void           UpdateRoomRects();

	void           DebugDraw_Pathmap(SDL_Surface* pDestSurface, MovementType eType);
	void           DebugDraw_MarkedTiles(SDL_Surface* pDestSurface);

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
	NOTICES           notices;

	std::map<const CMonster*, MonsterAnimation> monsterAnimations;
	bool              bShowingPlayer;   //whether player is visible onscreen
	bool              bShowCheckpoints;
	bool              bShowFrameRate, bShowMoveCount, bShowVarUpdates, bShowPuzzleMode;
	bool              bAddNEffect;   //for 'Neather striking orb
	bool              bRequestEvilEyeGaze; //for vision power-up
	bool              bRequestTranslucentTar;
	bool              bRequestSpiderVisibility;
	Uint8             ghostOpacity;
	int               temporalCloneEffectHeight;
	UINT              wHighlightX, wHighlightY; //user highlight position
	CMonster*         pHighlitMonster;
	CCoordSet         movingTLayerObjectsToRender;

	UINT              dwLastDrawSquareInfoUpdateCount;
	Uint32            dwLastAnimationFrame;
	Uint32            dwLastMonsterAnimation; //monster animation
	Uint32            dwLastBeethroAnimation;
	UINT              dwBeethroAnimationFrame;

	//For animating a turn
	UINT             dwMovementStepsLeft;
	Uint32            dwMoveDuration;   //duration of one movement
	Uint32            dwCurrentDuration;//total duration of all frames this turn
	Uint32				dwLastFrame, dwTimeSinceLastFrame; //time last frame was drawn (ms)
	bool              bFinishLastMovementNow;
	bool              bAnimateMoves, bAnimationInProgress;
	bool					bAllowSleep;		//whether hero can go to sleep (fidget)
	bool              bNextWispFrame;
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
	UINT              rain;             //rain drops are falling (0 = off)
	bool              bSkipLightfade;   //whether light crossfade is skipped
	WSTRING           sky;              //non-default sky image
	queue<UINT>       playThunder;      //when to play a thunder sound

	SDL_Surface      *pSkyImage;        //sky image
	WSTRING           wstrSkyImage;     //name of sky image

	//vars for optimization in rendering the room
	bool              bAllDirty;  //all room tiles must be redrawn
	bool              bWasPlacingDouble;   //player was placing double last frame
	bool              bWasInvisible;    //swordsman was invisible last frame
	UINT              wLastTurn;  //turn # at last frame
	int               displayFilterOverride; //takes precedence over CCurrentGame::displayFilter

	int               CX_TILE, CY_TILE;

private:
	void           AddPlatformPitMasks(const TileImageBlitParams& blit, t_PitMasks& pitMasks);
	void           AddTemporalCloneNextMoveEffect(const CTemporalClone *pTC, const UINT frame);
	inline void    ApplyDisplayFilter(int displayFilter, SDL_Surface* pDestSurface, UINT wX, UINT wY);
	void           ApplyDisplayFilterToRoom(int displayFilter, SDL_Surface *pDestSurface);
	inline int     getDisplayFilter() const;

	void           AnimateMonster(CMonster* pMonster);
	void           BlitTileShadowsOnMovingSprite(const TileImageBlitParams& blit, SDL_Surface *pDestSurface);

	void           CropAddLightParams(const SDL_Rect* crop,
		const SDL_Rect& roomEdgeClip,
	 	UINT& x, UINT& y, UINT& w, UINT& h,
		UINT& iStart, UINT& jStart, UINT& iEnd, UINT& jEnd,
		LIGHTTYPE*& sRGBIntensity) const;
	bool           CropTileBlitToRoomBounds(SDL_Rect* &crop, int dest_x, int dest_y) const;

	void           DisplayAgentsAffectingTiles(const CCoordSet& doorCoords);
	void           DisplayDoorAgents(const UINT wX, const UINT wY);
	bool           DisplayMonster(const CMonster* pMonster, const CharacterDisplayMode eDisplayMode) const;
	void           DrawGhostOverheadCharacters(SDL_Surface *pDestSurface, const bool bMoveInProgress=false);

	UINT           getApparentIdentity(const UINT logicalIdentity) const;

	CRoomEffectList* GetEffectListForLayer(const int layer) const;

	float          GetOverheadDarknessAt(const UINT wX, const UINT wY, const float fIntensity=1.0f) const;

	bool           GetPlayerDisplayTiles(const CSwordsman &swordsman,
			UINT& wO, UINT& wFrame, UINT& wSManTI, UINT& wSwordTI) const;

	bool           IsPersistentEffectPlaying(CEffectList* pEffectList, const UINT instanceID) const;
	bool           IsShowingBetterVision() const;
	bool           NeedsSwordRedrawing(const CMonster *pMonster) const;
	void           PopulateBuildMarkerEffects(const CDbRoom& room);

	void           PropagateLight(const float fSX, const float fSY, const float fZ,
			const UINT tParam, const bool bCenterOnTile=true,
			const Point& direction=Point(0.0f,0.0f,0.0f));
	void           PropagateLightNoModel(const int nSX, const int nSY, const UINT tParam);

	void           PlacePlayerLightAt(int pixel_x, int pixel_y);
	void           PropagatePlayerLight();
	void           PropagatePlayerLikeEntityLight(CEntity *pEntity);
	void           RemoveEffectsQueuedForRemoval();
	void           RenderPlayerLight();
	void           ResetPlayerLightMap();
	void           ResetUserLightsource();
	bool           ShowShadowCastingAnimation() const;
	bool           IsTemporalCloneLightRendered() const;

	void           flag_weather_refresh();
	void           SetFrameVarsForWeather();

	PuzzleModeOptions puzzleModeOptions;
	float             fDeathFadeOpacity;
	Uint32            time_of_last_weather_render;
	int               redrawingRowForWeather;
	bool              need_to_update_room_weather;

	Uint32            time_of_last_sky_move;

	multimap<EffectType, int> queued_layer_effect_type_removal;
};

#endif //#ifndef ROOMWIDGET_H

