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
 * JP Burford (jpburford), John Wm. Wicks (j_wicks), Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//DbRooms.h
//Declarations for CDbRooms, CDbRoom and several small data-containment classes.
//Class for accessing room data from database.

#ifndef DBROOMS_H
#define DBROOMS_H

#include "RoomData.h"

#include "DbVDInterface.h"
#include "DbDemos.h"
#include "DbSavedGames.h"
#include "ImportInfo.h"
#include "PlayerStats.h"

#include "Briar.h"
#include "Bridge.h"
#include "Building.h"
#include "GameConstants.h"
#include "Monster.h"
#include "MonsterFactory.h"
#include "Pathmap.h"
#include "Platform.h"
#include "Station.h"
#include "TileMask.h"
#include "NetInterface.h"

#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/CoordStack.h>

#include <list>

//******************************************************************************************
class CDbRooms;
class CCharacter;
class CDbSavedGame;
class CDbLevel;
class CDbSpeech;
class CDbDatum;
class CCueEvents;
class CPlayerDouble;
class CPlatform;
class CDbRoom : public CDbBase
{
protected:
	friend class CDbLevel;
	friend class CDbRooms;
	friend class CDbSavedGame;
	friend class CCurrentGame;
	friend class CDbVDInterface<CDbRoom>;

	CDbRoom();

	void           ClearPlotHistory();
	CCoordSet      PlotsMade;

public:
	CDbRoom(const CDbRoom &Src, const bool bCopyCurrentGame=true, const bool bCopyForEditor=false);
	CDbRoom &operator= (const CDbRoom &Src) {
		SetMembers(Src);
		return *this;
	}
	void MakeCopy(const CDbRoom &Src) {SetMembers(Src, false);}
	CDbRoom* MakeCopy(CImportInfo& info, const UINT newHoldID, const bool bCopyForEditor=false) const;

	virtual ~CDbRoom();

	UINT          dwRoomID;
	UINT          dwLevelID;
	UINT          dwRoomX;
	UINT          dwRoomY;
	UINT           wRoomCols;
	UINT           wRoomRows;
	WSTRING        style;
	bool           bIsRequired;   //room must be conquered to open blue doors
	bool           bIsSecret;     //room is marked as a secret
	UINT           dwDataID;      //for custom floor/wall/pit image texture
	UINT           wImageStartX, wImageStartY;   //where texture starts tiling
	UINT           dwOverheadDataID; //for custom overhead image texture
	UINT           wOverheadImageStartX, wOverheadImageStartY; //where texture starts tiling
	char *            pszOSquares;   //currently supports up to 256 tile types
	char *            pszFSquares;

	list<RoomObject*> tLayerObjects;

	CCoordIndex    overheadTiles;

	vector<COrbData*> orbs;
	vector<CScrollData*> Scrolls;
	vector<CExitData*>   Exits;
	CCoordSet      halphEnters, slayerEnters;   //where Halph/Slayer can enter the room
	CCoordSet      halph2Enters, slayer2Enters; //for alternate monster type
	CCoordSet      checkpoints;
	CDbPackedVars  ExtraVars;
	CMonster       *pFirstMonster, *pLastMonster;
	CCoordIndex_T<short> tileLights;       //per-tile lighting

	//In-game state information.
	RoomObject   **tLayer;
	CMonster **    pMonsterSquares;  //points to monster occupying each square
	UINT           wMonsterCount;
	UINT           wBrainCount;
	UINT           wTarLeft;
	UINT           wTrapDoorsLeft;
	bool		   bTarWasBuilt;	//Whether tar was built this turn and the unstable tar needs to be cleared
	bool           bBetterVision;    //sight token
	bool           bPersistentCitizenMovement; //citizen movement algorithm
	bool           bHasConquerToken;   //whether a Conquer token is in the room
	bool           bHasActiveBeacon;   //whether a Seeding Beacon has reseeded the room
	CMonster      *pLastClone;       //last clone queried


	CPathMap *     pPathMap[NumMovementTypes];
	CCoordSet      LitFuses, NewFuses;  //all fuse pieces (and bombs) burning this turn
	CCoordSet      Beacons;   //all beacons in the room
	CCoordStack    NewBabies; //unstable tar tiles marked for baby conversion
	CBriars        briars;   //briar roots in the room
	list<CPlayerDouble*> Decoys, monsterEnemies;  //player decoys, monster enemies in the room
	vector<CPlatform*>   platforms;  //all moving platforms in the room
	vector<CStation*>    stations;   //all relay stations in the room
	CCoordIndex    coveredOSquares;  //what is under removable o-tile objects
	CCoordIndex_T<USHORT> pressurePlateIndex; //which pressure plate is on this square
	bool				bTarWasStabbed;	//for "dangerous room" heuristic
	bool				bGreenDoorsOpened;	//when all monsters have been killed
	Weather        weather;	//environmental weather conditions
	CBridge        bridges;
	CBuilding      building; //tiles marked for building
	FloorSpikes    floorSpikes;
	CCoordSet      fluffVents;
	CCoordSet      activeFiretraps; //all active firetraps in the room

	CCoordSet      geometryChanges, disabledLights; //for front end -- where lighting must be updated

	//Uniform way of accessing 2D information in 1D array (column-major).
	inline UINT    ARRAYINDEX(const UINT x, const UINT y) const {return (y * this->wRoomCols) + x;}

	void           ActivateBeacon(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           ActivateFiretrap(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	void           ActivateOrb(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const OrbActivationType eActivationType, const bool bBreakOrb = true);
	void           ActivateToken(CCueEvents &CueEvents, const UINT wX, const UINT wY, CPlayerDouble* pDouble=NULL);
	void           AddDiagonalDoorAssociations();
	bool           AddFallingMonsterEvent(CCueEvents& CueEvents, CMonster* pMonster, const UINT wOSquare) const;
	CMonster *     AddNewMonster(const UINT wMonsterType, const UINT wX,
			const UINT wY, const bool bInRoom=true, const bool bLinkMonster=true);
	bool           AddOrb(COrbData *pOrb);
	COrbData *     AddOrbToSquare(const UINT wX, const UINT wY);
	bool           AddPressurePlateTiles(COrbData* pPlate);
	bool           AddNewGlobalScript(const UINT dwCharID, CCueEvents &CueEvents);
	void           AddRunningGlobalScripts(CCueEvents &CueEvents);
	bool           AddScroll(CScrollData *pScroll);
	bool           AddExit(CExitData *pExit);

	bool           AllocTileLayers();

	void           BurnFuses(CCueEvents &CueEvents);
	void           BurnFuseEvents(CCueEvents &CueEvents);
	bool           BrainSensesSwordsman() const;
	bool           BrainSensesTarget() const;
	inline UINT    CalcRoomArea() const {return this->wRoomCols * this->wRoomRows;}

	bool    CanPushOntoFTile(const UINT wFromX, const UINT wFromY, const UINT wToX, const UINT wToY) const;
	bool    CanPushOntoTTile(const UINT wX, const UINT wY) const;
	bool    CanPushOntoOTile(const UINT wX, const UINT wY) const;

	bool           CanMovePlatform(const UINT wX, const UINT wY, const UINT wO);
	bool           CanSetSwordsman(const UINT dwX, const UINT dwY,
			const bool bRoomConquered=true) const;
	bool           CanEndHoldHere() const;
	bool           CanPlayerMoveOnThisElement(const UINT wAppearance, const UINT wTileNo) const;
	bool           CanPushTo(const UINT wFromX, const UINT wFromY,
			const UINT wX, const UINT wY) const;
	bool           CanPushMonster(const CMonster* pMonster,
			const UINT wFromX, const UINT wFromY, const UINT wToX, const UINT wToY) const;

	void           ChangeTiles(const RoomTokenType tType);
	void           CharactersCheckForCueEvents(CCueEvents &CueEvents);
	void           CheckForFallingAt(const UINT wX, const UINT wY, CCueEvents& CueEvents, bool bTrapdoorFell=false);
	void           ClearDeadMonsters();
	void           ClearDeadRoomObjects();
	void           ClearMonsters(const bool bRetainNonConquerableMonsters=false);
	void           ClearPlatforms();
	void           ClearTLayer();
	void           CreatePathMap(const UINT wX, const UINT wY,
			const MovementType eMovement);
	void           CreatePathMaps();
	bool           CropRegion(UINT& x1, UINT &y1, UINT &x2, UINT &y2) const;
	int            DangerLevel() const;
	void           DeactivateBeacons();
	void           DecMonsterCount();
	void           DecTrapdoor(CCueEvents &CueEvents);
	void           DeleteExitAtSquare(const UINT wX, const UINT wY);
	void           DeleteOrbAtSquare(const UINT wX, const UINT wY);
	void           DeleteScrollTextAtSquare(const UINT wX, const UINT wY);
	void           DestroyCrumblyWall(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const UINT wStabO=NO_ORIENTATION);
	void           DestroyFluff(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           DestroyTar(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           DestroyTrapdoor(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           DisableFiretrap(const UINT wX, const UINT wY);
	void           DisableForceArrow(const UINT wX, const UINT wY);
	void           DisableToken(CCueEvents& CueEvents, UINT x, UINT y);
	void           DoExplode(CCueEvents &CueEvents, CCoordStack& bombs, CCoordStack& powder_kegs);
	void           DoExplodeTile(CCueEvents& CueEvents, CCoordStack& bombs, CCoordStack& powder_kegs,
			CCoordSet& explosion, const CCoordIndex& caberCoords, UINT wCol, UINT wRow, UINT explosion_radius,
			bool bAddCueEvent=true);
	bool           DoesGentryiiPreventDiagonal(const UINT x1, const UINT y1,
			const UINT x2, const UINT y2) const;
	bool           DoesMonsterEnterRoomLater(const UINT wX, const UINT wY,
			const UINT wMonsterType) const;
	bool           DoesSquareContainDoublePlacementObstacle(const UINT wX, const UINT wY,
			const UINT wDoubleType=M_MIMIC) const;
	bool           DoesSquareContainPlayerObstacle(const UINT wX, const UINT wY,
			const UINT wO, bool& bMonsterObstacle) const;
	bool           DoesSquarePreventDiagonal(const UINT wX, const UINT wY,
			const int dx, const int dy) const;
	bool           DoesSquareContainTeleportationObstacle(const UINT wX, const UINT wY, const UINT wIdentity) const;
	bool           DoubleCanActivateToken(RoomTokenType type) const;

	void           EnableFiretrap(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           EnableForceArrow(const UINT wX, const UINT wY);
	void           ExpandExplosion(CCueEvents &CueEvents, CCoordStack& cs, const UINT wBombX, const UINT wBombY, const UINT wX, const UINT wY,
			CCoordStack& bombs, CCoordStack& powder_kegs, CCoordSet& explosion, const CCoordIndex& caberCoords, const UINT radius);
	void           ExplodeBomb(CCueEvents &CueEvents, const UINT x, const UINT y) { CCoordStack bombs(x,y); CCoordStack cs; DoExplode(CueEvents, bombs, cs); }
	void           ExplodePowderKeg(CCueEvents &CueEvents, const UINT x, const UINT y) { CCoordStack cs; CCoordStack powder_kegs(x,y); DoExplode(CueEvents, cs, powder_kegs); }

	CMonster*      FindNextClone();
	void           FindOrbsToOpenDoor(CCoordSet& orbs, const CCoordSet& doorSquares) const;
	void           FindPlatesToOpenDoor(CCoordSet& plateTiles, const CCoordSet& doorSquares) const;
	void           FixUnstableTar(CCueEvents &CueEvents);
	void           BreakUnstableTar(CCueEvents &CueEvents);
	void           FloodPlot(const UINT wX, const UINT wY, const UINT wNewTile,
			const bool b8Neighbor=true);
	void           GetAllYellowDoorSquares(const UINT wX, const UINT wY, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL) const;
	void           GetCaberCoords(CCoordIndex &CaberCoords) const;
	CCharacter*    GetCharacterWithScriptID(const UINT scriptID);
	void           GetConnectedRegionsAround(const UINT wX, const UINT wY,
			const CTileMask &tileMask, vector<CCoordSet>& regions,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const;

	enum TileConnectionStrategy { Connect_4, Connect_8, Connect_8_WithoutAxialPit };
	void           GetConnected4NeighborTiles(const UINT wX, const UINT wY,
			const CTileMask &tileMask, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const
		{ GetConnectedTiles(wX, wY, tileMask, Connect_4, squares, pIgnoreSquares, pRegionMask); }
	void           GetConnected8NeighborTiles(const UINT wX, const UINT wY,
			const CTileMask &tileMask, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const
		{ GetConnectedTiles(wX, wY, tileMask, Connect_8, squares, pIgnoreSquares, pRegionMask); }
	void           GetConnected8NeighborTilesWithoutAxialPit(const UINT wX, const UINT wY,
			const CTileMask &tileMask, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const
		{ GetConnectedTiles(wX, wY, tileMask, Connect_8_WithoutAxialPit, squares, pIgnoreSquares, pRegionMask); }

	CCurrentGame*  GetCurrentGame() const {return this->pCurrentGame;}
	UINT           GetExitIndexAt(const UINT wX, const UINT wY) const;
	bool           GetExitEntranceIDAt(const UINT wX, const UINT wY, UINT &dwEntranceID) const;
	FloorSpikesType GetFloorSpikesState() const;
	UINT          GetImportCharacterSpeechID();
	CDbLevel*      GetLevel() const;
	void           GetLevelPositionDescription(WSTRING &wstrDescription,
			const bool bAbbreviate=false);
	void           GetDoubleSwordCoords(CCoordIndex &DoubleSwordCoords,
			const bool bIgnoreDagger = false, const bool bIgnoreNonCuts = false, CMonster *pIgnore=NULL) const;
	void           GetPositionInLevel(int& dx, int& dy) const;
	UINT           GetPrimaryKey() const {return this->dwRoomID;}
	void           getStats(RoomStats& stats, const CDbLevel *pLevel) const;
	UINT           GetSquarePathMapObstacles(const UINT wX, const UINT wY, const MovementType eMovement) const;
	void           GetSwordCoords(CCoordIndex &SwordCoords,
			const bool bIgnoreDagger = false, const bool bIgnoreNonCuts = false, CMonster *pIgnore=NULL) const;
	CMonster*      GetMonsterAtSquare(const UINT wX, const UINT wY) const;
	const CMonster* GetOwningMonsterOnSquare(const UINT wX, const UINT wY) const;
	CMonster*      GetMonsterOfType(const UINT wType) const;
	UINT           GetMonsterTypeAt(const UINT wX, const UINT wY,
			const bool bConsiderNPCIdentity=false, const bool bOnlyLiveMonsters=true) const;
	bool           GetNearestEntranceTo(const UINT wX, const UINT wY, const MovementType eMovement, UINT &wEX, UINT &wEY);
	CMonster*      GetNPCBeethro(bool bDeadOnly=false) const;
	COrbData*      GetOrbAtCoords(const UINT wX, const UINT wY) const;
	void           GetDepressablePlateSubset(const CCoordSet& plates,
			CCoordSet& depressablePlates) const;
	COrbData*      GetPressurePlateAtCoords(const UINT wX, const UINT wY) const;
	const WCHAR*   GetScrollTextAtSquare(const UINT wX, const UINT wY) const;
	CScrollData*   GetScrollAtSquare(const UINT wX, const UINT wY) const;
	UINT           GetOSquare(const UINT wX, const UINT wY) const;
	UINT           GetFSquare(const UINT wX, const UINT wY) const;
	UINT           GetTSquare(const UINT wX, const UINT wY) const;
	UINT           GetTSquare(const UINT index) const;
	UINT           GetBottomTSquare(const UINT wX, const UINT wY) const;
	UINT           GetCoveredTSquare(const UINT wX, const UINT wY) const;
	UINT           GetCoveredTSquare(const UINT index) const;
	const RoomObject* GetTObject(const UINT index) const { return tLayer[index]; }
	const RoomObject* GetTObject(const UINT wX, const UINT wY) const { return GetTObject(ARRAYINDEX(wX, wY)); }
	UINT           GetTParam(const UINT wX, const UINT wY) const;
	UINT           GetTParam(const UINT index) const;
	UINT           GetOSquareWithGuessing(const int nX, const int nY) const;
	UINT           GetTSquareWithGuessing(const int nX, const int nY) const;
	CEntity*       GetSpeaker(const UINT wType, const bool bConsiderBaseType=false);
	CPlatform*     GetPlatformAt(const UINT wX, const UINT wY) const;
	void           GetTarConnectedComponent(const UINT wX, const UINT wY,
			CCoordSet& tiles, const bool bAddAdjOnly=false) const;
	void           GrowTar(CCueEvents &CueEvents, CCoordIndex &babies,
			CCoordIndex &SwordCoords, const UINT wTarType);
	void           ForceTileRedraw(const UINT wX, const UINT wY, const bool bGeometryChanges);
	void           IncTrapdoor(CCueEvents& CueEvents);
	void           InitCoveredTiles();
	void           InitRoomStats();
	void           InitStateForThisTurn();
	bool           IsAnyTLayerObject(const UINT wX, const UINT wY, const UINT tile) const;
	bool           IsBeaconActive() const;
	inline bool    IsBrainPresent() const {return this->wBrainCount != 0;}
	bool           IsDoorOpen(const int nCol, const int nRow);
	static bool    IsGrowingTarstuffMotherAlive(CCueEvents &CueEvents, CUEEVENT_ID cid);
	bool           IsOrbBeingStruck(const UINT wX, const UINT wY) const;
	bool           IsMonsterInRect(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const bool bConsiderPieces=true) const;
	bool           IsMonsterInRectOfType(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const UINT wType,
			const bool bConsiderNPCIdentity=false, const bool bUseNPCLogicalIdentity=false) const;
	bool           IsMonsterNextTo(const UINT wX, const UINT wY, const UINT wType) const;
	bool           IsMonsterOfTypeAt(const UINT eType, const UINT wX, const UINT wY,
			const bool bConsiderNPCIdentity=false, const bool bOnlyLiveMonsters=true) const;
	bool           IsMonsterWithin(const UINT wX, const UINT wY,
			const UINT wSquares, const bool bConsiderPieces=true) const;
	bool           IsMonsterSwordAt(const UINT wX, const UINT wY,
			const bool bIgnoreDagger=false, const CMonster *pIgnore=NULL) const;
    bool           IsMonsterWeaponTypeAt(const UINT wX, const UINT wY,
        const WeaponType wt, const CMonster *pIgnore = NULL) const;
	bool           IsPathmapNeeded() const;
	bool           IsTimerNeeded() const;
	static bool    IsRequired(const UINT dwRoomID);
	bool           IsRoomLightingChanged() const { return room_lighting_changed; }
	static bool    IsSecret(const UINT dwRoomID);
	bool           IsSwordAt(const UINT wX, const UINT wY) const;
	bool           IsSwordWithinRect(const UINT wMinX, const UINT wMinY,
			const UINT wMaxX, const UINT wMaxY) const;
	bool           IsTarStableAt(const UINT wX, const UINT wY, const UINT wTarType) const;
	bool           IsTarVulnerableToStab(const UINT wX, const UINT wY) const;
	bool           IsTileInRectOfType(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const UINT wType) const;
	bool           IsValidColRow(const UINT wX, const UINT wY) const;
	void           KillFluffOnHazard(CCueEvents &CueEvents);
	void           KillMonstersOnHazard(CCueEvents &CueEvents);
	bool           KillMonster(CMonster *pMonster, CCueEvents &CueEvents,
			const bool bForce=false, const CEntity* pKillingEntity=NULL);
	bool           KillMonsterAtSquare(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const bool bForce=false);
	void           LightFuse(CCueEvents &CueEvents, const UINT wCol, const UINT wRow);
	void           LightFuseEnd(CCueEvents &CueEvents, const UINT wCol, const UINT wRow);
	void           LinkMonster(CMonster *pMonster, const bool bInRoom=true, const bool bReverseRule=false);
	void           UnlinkMonster(CMonster *pMonster);
	bool           Load(const UINT dwLoadRoomID, const bool bQuick=false);
	CMonster*      LoadMonster(const c4_RowRef& row, CDbHold* &pHold);
	bool           LoadTiles();
	void           MarkSpeechForDeletion(CDbSpeech* pSpeech);
	void           MarkDataForDeletion(const CDbDatum* pDatum);
	void           MarkPlotImageTiles();
	void           MarkPlotOverheadTiles();
	bool           MonsterHeadIsAt(const UINT wX, const UINT wY) const;
	CMonster*      MonsterOfTypeExists(const UINT eType) const;
	bool           MonsterWithMovementTypeExists(const MovementType eMovement) const;
	void           MoveMonster(const CMonster* pMonster, const UINT wDestX, const UINT wDestY);
	void           MovePlatform(const UINT wX, const UINT wY, const UINT wO);
	void           MoveScroll(const UINT wX, const UINT wY,
			const UINT wNewX, const UINT wNewY);
	void           PlaceCharacters(CDbHold* pHold=NULL);
	void           Plot(const UINT wX, const UINT wY, const UINT wTileNo,
			CMonster *pMonster=NULL, bool bUnderObject=false);
	void           Plot(const CCoordSet& plots, const bool bChangesRoomGeometry=false);
	void           PlotMonster(UINT wX, UINT wY, UINT wTileNo, CMonster *pMonster);
	void           PreprocessMonsters(CCueEvents& CueEvents);
	static bool    PressurePlateIsDepressedBy(const UINT item);
	void           ProcessTurn(CCueEvents &CueEvents, const bool bFullMove);
	void           PostProcessTurn(CCueEvents &CueEvents, const bool bFullMove);
	void           ProcessExplosionSquare(CCueEvents &CueEvents, const UINT wX, const UINT wY,
			const bool bKillsFegundo=true, const bool bAddCueEvent=true);
	void           ProcessAumtlichGaze(CCueEvents &CueEvents, const bool bFullMove);
	void           ProcessPuffAttack(CCueEvents &CueEvents, const UINT wX, const UINT wY);
	void           PushTLayerObject(const UINT wSrcX, const UINT wSrcY,
			const UINT wDestX, const UINT wDestY, CCueEvents& CueEvents);
	void           RecalcStationPaths();
	void           ReevalBriarNear(const UINT wX, const UINT wY, const UINT wTileNo);
	void           ReflectX();
	void           ReflectY();
	void           Reload();
	void           RemoveCoveredTLayerItem(const UINT wSX, const UINT wSY);
	void           RemoveFinishedCharacters();
	void           RemoveFuse(const UINT wCol, const UINT wRow);
	void           RemoveMonsterDuringPlayWithoutEffect(CMonster* pMonster);
	void           RemoveMonsterFromTileArray(CMonster* pMonster);
	bool           RemovePressurePlateTile(const UINT wX, const UINT wY);
	void           RemoveStabbedTar(const UINT wX, const UINT wY, CCueEvents &CueEvents, const bool bKillBaby=true);
	void           ConvertUnstableTar(CCueEvents &CueEvents, const bool bDelayBabyMoves=false);
	bool           RemoveTiles(const UINT wOldTile);
	void           RemoveYellowDoorTile(const UINT wX, const UINT wY,
			const UINT wTile);
	void           ResetExitIDs();
	static void    ResetForImport();
	void           ResetMonsterFirstTurnFlags();
	void           ResetPressurePlatesState();
	void           SetCoveredTLayerObject(const UINT wX, const UINT wY, const UINT tile);
	void           SetCurrentGame(CCurrentGame *const pSetCurrentGame);

	void           SetHalphSlayerEntrance();
	void           SetMonsterSquare(CMonster *pMonster);
	void           SetPathMapsTarget(const UINT wX, const UINT wY);
	void           SetPressurePlatesState();
	void           SetRoomLightingChanged() { room_lighting_changed = true; }
	void           SetTParam(const UINT wX, const UINT wY, const BYTE value);

	//Import handling
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);

	void SetRoomEntryState(CCueEvents& CueEvents, const bool bWasLevelComplete,
			const bool bIsCurrentLevelComplete, const bool bWasRoomConquered,
			UINT& wMonsterCountAtStart);
	void           SetScrollTextAtSquare(const UINT wX, const UINT wY, const WCHAR* pwczScrollText);
	void           SetExit(const UINT dwEntranceID, const UINT wX, const UINT wY,
			const UINT wX2=(UINT)-1, const UINT wY2=(UINT)-1);
	bool           SomeMonsterCanSmellSwordsman() const;
	bool           StabTar(const UINT wX, const UINT wY, CCueEvents &CueEvents,
			const bool removeTarNow, const UINT wStabO=NO_ORIENTATION);
	static int     SwapTarstuffRoles(const UINT type, const bool bTar, const bool bMud, const bool bGel);
	void           SwitchTarstuff(const UINT wType1, const UINT wType2, CCueEvents& CueEvents);
	WeaponType     SwordfightCheck() const;
	void           ToggleBlackGates(CCueEvents& CueEvents);
	void           ToggleFiretrap(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           ToggleForceArrow(const UINT wX, const UINT wY);
	bool           ToggleGreenDoors(CCueEvents& CueEvents);
	bool           ToggleTiles(const UINT wOldTile, const UINT wNewTile, CCueEvents& CueEvents);
	void           ToggleLight(const UINT wX, const UINT wY);
	void           TurnOffLight(const UINT wX, const UINT wY);
	void           TurnOnLight(const UINT wX, const UINT wY);
	virtual bool   Update();
	void           UpdatePathMapAt(const UINT wX, const UINT wY);
	bool           WasObjectPushedThisTurn(const UINT wX, const UINT wY) const
			{ return this->pushed_objects.count(this->tLayer[ARRAYINDEX(wX, wY)]) != 0; }
	bool           WasMonsterPushedThisTurn(const CMonster* pMonster) const
			{ return this->pushed_monsters.count(pMonster) != 0; }

	//scope: used while processing the current turn
	set<const RoomObject*> pushed_objects;
	set<const CMonster*> pushed_monsters;
	bool pushed_player;

private:
	enum tartype {oldtar, newtar, notar};

	void           AddPlatformPiece(const UINT wX, const UINT wY, CCoordIndex &plots);
	RoomObject*    AddTLayerObject(const UINT wX, const UINT wY, const UINT tile);

	void           CalcFluffGrowth(const CSwordsman& player, const UINT wSManX, const UINT wSManY,
			CCoordSet& newGrowth, CCoordSet& newPuffs, CCueEvents &CueEvents);

	void           Clear();
	void           ClearPushStates();
	void           ClearStateVarsUsedDuringTurn();
	void           CloseYellowDoor(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           CopyTLayer(const list<RoomObject*>& src);
	void           DeletePathMaps();
	CCoordStack    GetPowderKegsStillOnHotTiles() const;
	void           ExplodeStabbedPowderKegs(CCueEvents& CueEvents);
	UINT           FuseEndAt(const UINT wCol, const UINT wRow, const bool bLighting=true) const;
	UINT           GentryiiFallsInPit(UINT wPrevX, UINT wPrevY,
			MonsterPieces::iterator pieceIt, MonsterPieces::const_iterator pieces_end,
			CCueEvents& CueEvents);
	void           GetConnectedTiles(const UINT wX, const UINT wY,
			const CTileMask &tileMask, const TileConnectionStrategy eConnect, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const;
	void           GetLevelPositionDescription_English(WSTRING &wstrDescription,
			const int dx, const int dy, const bool bAbbrev=false);
	void           GetLevelPositionDescription_Russian(WSTRING &wstrDescription,
			const int dx, const int dy, const bool bAbbrev=false);
	UINT           GetLocalID() const;
	void           GetNumber_English(const UINT num, WCHAR *str);
	bool           LargeMonsterFalls(CMonster* &pMonster, const UINT wX, const UINT wY, CCueEvents& CueEvents);
	bool           LoadOrbs(c4_View &OrbsView);
	bool           LoadMonsters(c4_View &MonstersView);
	bool           LoadScrolls(c4_View &ScrollsView);
	bool           LoadExits(c4_View &ExitsView);
	bool           LoadCheckpoints(c4_View &CheckpointsView);
	void           MarkTilesForStyle(const CTileMask &mask,
			UINT* const tileTypes, const UINT numTileTypes, const UINT wMajorityStyle);
	void           MarkTilesFromSquare(const UINT wX, const UINT wY,
			const CTileMask &mask, UINT* const tileTypes, const UINT numTileTypes,
			const UINT wMajorityStyle, const bool b8Neighbor);
	bool           NewGelWouldBeStable(const vector<tartype> &addedGel, const UINT tx, const UINT ty, const CCoordSet& contiguousGel);
	bool           NewTarWouldBeStable(const vector<tartype> &addedTar, const UINT tx, const UINT ty);
	void           ObstacleFill(CCoordIndex& obstacles);
	void           OpenYellowDoor(const UINT wX, const UINT wY);
	c4_Bytes *     PackSquares() const;
	c4_Bytes *     PackTileLights() const;
	void           ProcessActiveFiretraps(CCueEvents &CueEvents);
	void           ProcessFluffVents(CCueEvents &CueEvents);
	void           ProcessSpikes(CCueEvents &CueEvents, const bool bNoStab=false);
	void           ReflectSquare(const bool bHoriz, UINT &wSquare) const;
	void           RemoveDecoy(CMonster *pMonster);
	bool           RemoveLongMonsterPieces(CMonster *pMonster);
	void           RemoveMonsterEnemy(CMonster *pMonster);
	void           RemoveMonsterFromLayer(CMonster *pMonster);
	void           RemoveTLayerObject(RoomObject* obj);
	void           ReplaceOLayerTile(const UINT wX, const UINT wY, const UINT wTileNo);
	void           ReplaceTLayerItem(const UINT wX, const UINT wY, const UINT wTileNo, const bool bUnderObject);
	void           ResetUnloadedPressurePlates(CCueEvents &CueEvents);
	void           SaveOrbs(c4_View &OrbsView) const;
	void           SaveMonsters(c4_View &MonstersView);
	void           SaveScrolls(c4_View &ScrollsView);
	void           SaveExits(c4_View &ExitsView) const;
	void           SaveCheckpoints(c4_View &CheckpointsView) const;
	void           SetCurrentGameForMonsters(const CCurrentGame *pSetCurrentGame);
	void           SetExtraVarsFromMembers();
	bool           SetMembers(const CDbRoom &Src, const bool bCopyLocalInfo=true, const bool bCopyCurrentGame=true, const bool bCopyForEditor=false);
	void           SetMembersFromExtraVars();
	RoomObject*    SetTLayer(const UINT wX, const UINT wY, const UINT tile);
	void           SolidifyFluff(CCueEvents &CueEvents);
	void           swapTLayer(const UINT x1, const UINT y1, const UINT x2, const UINT y2);
	void           ToggleYellowDoor(const UINT wX, const UINT wY, CCueEvents &CueEvents);

	bool           UnpackSquares(const BYTE *pSrc, const UINT dwSrcSize);
	bool           UnpackSquares1_6(const BYTE *pSrc, const UINT dwSrcSize);
	bool           UnpackTileLights(const BYTE *pSrc, const UINT dwSrcSize);

	bool           UpdateExisting();
	bool           UpdateNew();
	void           UpdateFields(c4_RowRef& row);

	list<CMonster *>  DeadMonsters;
	list<RoomObject*> DeadRoomObjects;

	vector<UINT> deletedScrollIDs;  //message text IDs to be deleted on Update
	vector<UINT> deletedSpeechIDs;  //speech IDs to be deleted on Update
	vector<UINT> deletedDataIDs;    //data IDs to be deleted on Update

	CCurrentGame * pCurrentGame;
	bool           bCheckForHoldCompletion;
	bool           bCheckForHoldMastery;

	//scope: used while processing the current turn
	set<const RoomObject*> stationary_powder_kegs;
	set<const CMonster*> monsters_stabbed_by_spikes_this_turn;
	CCoordStack stabbed_powder_kegs;
	bool room_lighting_changed;
};

//******************************************************************************************
class CDbRooms : public CDbVDInterface<CDbRoom>
{
protected:
	friend class CDbLevel;
	friend class CDb;
	CDbRooms()
		: CDbVDInterface<CDbRoom>(V_Rooms, p_RoomID),
		  dwFilterByLevelID(0)
	{ }

public:
	virtual void      Delete(const UINT dwRoomID);
	virtual bool   ExportText(const UINT dwRoomID, CDbRefs &dbRefs, CStretchyBuffer &str);
	virtual void ExportXML(const UINT dwRoomID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void     ExportNetRoom(const UINT dwRoomID, CNetRoom& room);
	void     FilterBy(const UINT dwSetFilterByLevelID);
	static UINT      FindIDAtCoords(const UINT dwLevelID, const UINT dwRoomX,
			const UINT dwRoomY);
	static UINT		GetAuthorID(const UINT dwRoomID);
	static CDbRoom *  GetByCoords(const UINT dwLevelID, const UINT dwRoomX,
			const UINT dwRoomY);
	static UINT      GetHoldIDForRoom(const UINT dwRoomID);
	static UINT      GetLevelIDForRoom(const UINT dwRoomID);
	virtual CDbRoom * GetNew();

	void        LogRoomsWithItem(const UINT wTile, const UINT wParam=0);

private:
	virtual void      LoadMembership();

	UINT    dwFilterByLevelID;
};

bool bIsArrowObstacle(const UINT nArrowTile, const UINT nO);

#endif //...#ifndef DBROOMS_H
