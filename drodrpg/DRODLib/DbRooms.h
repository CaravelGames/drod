// $Id: DbRooms.h 10216 2012-05-20 08:36:59Z skell $

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
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DbVDInterface.h"
#include "DbDemos.h"
#include "DbSavedGames.h"

#include "Bridge.h"
//#include "Building.h"
#include "Briar.h"
#include "GameConstants.h"
#include "Monster.h"
#include "MonsterFactory.h"
#include "Pathmap.h"
#include "Platform.h"
#include "PlayerStats.h"
//#include "Station.h"
#include "TileMask.h"

#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/CoordStack.h>

#include <list>

#define NO_EXIT  static_cast<UINT>(-1)

//******************************************************************************************
//What type of object is activating an orb.
enum OrbActivationType
{
	OAT_Player = 0,
	OAT_Monster = 1,
	OAT_Item = 2,
	OAT_ScriptOrb = 3,
	OAT_PressurePlate = 4,
	OAT_PressurePlateUp = 5,
	OAT_ScriptPlate = 6
};

//Orb agent types.
enum OrbAgentType
{
	OA_NULL = 0,
	OA_TOGGLE = 1,
	OA_OPEN = 2,
	OA_CLOSE = 3,
	OA_COUNT
};

//Orb/pressure plate types.
enum OrbType
{
	OT_NORMAL = 0,
	OT_ONEUSE = 1,
	OT_TOGGLE = 2,
	OT_BROKEN = 3,
	OT_COUNT
};

static inline bool bIsValidOrbAgentType(const OrbAgentType action)
{return action > OA_NULL && action < OA_COUNT;}
static inline bool bIsValidOrbType(const OrbType type)
{return type >= OT_NORMAL && type < OT_COUNT;}

static inline OrbAgentType operator--(OrbAgentType &d) { return d=static_cast<OrbAgentType>(d-1);}
static inline OrbAgentType operator--(OrbAgentType &d, int) { OrbAgentType const t=d; --d; return t; }
static inline OrbAgentType operator++(OrbAgentType &d) { return d=static_cast<OrbAgentType>(d+1);}
static inline OrbAgentType operator++(OrbAgentType &d, int) { OrbAgentType const t=d; ++d; return t; }

class COrbAgentData : public CCoord
{
public:
	COrbAgentData() : CCoord(0,0), action(OA_NULL)
	{ }
	COrbAgentData(const UINT wX, const UINT wY, const OrbAgentType action)
		: CCoord(wX, wY), action(action) { }
	COrbAgentData(const COrbAgentData* src)
		: CCoord(src->wX, src->wY), action(src->action) { }

	OrbAgentType   action;
};

//******************************************************************************************
class COrbData : public CCoord
{
public:
	COrbData()
		: CCoord(0,0), eType(OT_NORMAL), bActive(false)
	{ }
	~COrbData()
	{
		ClearAgents();
	}

	COrbData(const UINT wX, const UINT wY)
		: CCoord(wX, wY), eType(OT_NORMAL), bActive(false) { }
	COrbData(const COrbData &Src) : CCoord() {SetMembers(Src);}
	COrbData &operator= (const COrbData &Src)
	{
		SetMembers(Src);
		return *this;
	}
	void ClearAgents()
	{
		UINT wIndex=this->agents.size();
		while (wIndex--)
			delete this->agents[wIndex];
		this->agents.clear();
	}

	COrbAgentData* AddAgent(const UINT wX, const UINT wY, const OrbAgentType action)
	{
		ASSERT(bIsValidOrbAgentType(action));
		COrbAgentData *pNewAgent = new COrbAgentData(wX, wY, action);
		this->agents.push_back(pNewAgent);
		return pNewAgent;
	}

	void AddAgent(COrbAgentData *pAgent)
	{
		this->agents.push_back(pAgent);
	}

	bool DeleteAgent(COrbAgentData* const pOrbAgent)
	{
		ASSERT(pOrbAgent);
		UINT wAgentI;
		for (wAgentI=this->agents.size(); wAgentI--; )
			if (pOrbAgent == this->agents[wAgentI])
			{
				//Found it.  Remove agent (replace with last one).
				delete this->agents[wAgentI];
				this->agents[wAgentI] = this->agents[this->agents.size()-1];
				this->agents.pop_back();
				return true;
			}
		return false;  //didn't find it
	}

	COrbAgentData* GetAgentAt(const UINT wX, const UINT wY) const
	{
		for (UINT wAgentI=0; wAgentI<this->agents.size(); ++wAgentI)
			if (wX == this->agents[wAgentI]->wX && wY == this->agents[wAgentI]->wY)
				return this->agents[wAgentI];

		return NULL;   //didn't find it
	}

	vector<COrbAgentData*>  agents;
	OrbType eType;

	//for pressure plates
	bool bActive;    //currently depressed
	CCoordSet tiles; //coords comprising the pressure plate

private:
	void SetMembers(const COrbData &Src)
	{
		ClearAgents();
		this->wX = Src.wX;
		this->wY = Src.wY;
		this->eType = Src.eType;
		this->bActive = Src.bActive;
		this->tiles = Src.tiles;
		for (UINT wAgentI=0; wAgentI < Src.agents.size(); ++wAgentI)
			this->agents.push_back(new COrbAgentData(Src.agents[wAgentI]));
	}
};

//******************************************************************************************
class CScrollData : public CCoord
{
public:
	CScrollData() : CCoord(0,0)
	{ }

	CScrollData(CScrollData &Src) : CCoord() {SetMembers(Src);}
	CScrollData& operator= (CScrollData &Src)
	{
		SetMembers(Src);
		return *this;
	}

	CDbMessageText ScrollText;

private:
	void SetMembers(CScrollData &Src)
	{
		wX = Src.wX;
		wY = Src.wY;
		ScrollText = (const WCHAR*)Src.ScrollText;
	}
};

//******************************************************************************************
class CExitData
{
public:
	CExitData()
		: dwEntranceID(0), wLeft(0), wRight(0), wTop(0), wBottom(0)
	{ }
	CExitData(const UINT dwEntranceID, const UINT wLeft, const UINT wRight,
			const UINT wTop, const UINT wBottom)
		: dwEntranceID(dwEntranceID), wLeft(wLeft), wRight(wRight), wTop(wTop), wBottom(wBottom)
	{ }
	UINT       dwEntranceID;
	UINT        wLeft, wRight, wTop, wBottom;
};

//******************************************************************************************
//Types of room tokens.
enum RoomTokenType
{
	RotateArrowsCW=0,
	RotateArrowsCCW=1,
	SwitchTarMud=2,
	SwitchTarGel=3,
	SwitchGelMud=4,
	TarTranslucent=5,
	SwordDisarm=6,
//	PowerTarget=7,
//	PersistentCitizenMovement=8,
	RoomTokenCount
};

//Types of door keys.
enum KeyType
{
	YellowKey=0,
	GreenKey=1,
	BlueKey=2,
	SkeletonKey=3,
	KeyCount
};

//Types of swords.
enum SwordType
{
	NoSword=0,
	WoodenBlade=1,
	ShortSword=2,
	GoblinSword=3,
	LongSword=4,
	HookSword=5,
	ReallyBigSword=6,
	LuckySword=7,
	SerpentSword=8,
	BriarSword=9,
	WeaponSlot=10,  //allows placing the held weapon at this location
	SwordCount
};

enum ShieldType
{
	NoShield=0,
	WoodenShield=1,
	BronzeShield=2,
	SteelShield=3,
	KiteShield=4,
	OremiteShield=5,
	ArmorSlot=6,
	ShieldCount,
	ShieldSwordOffset=8 //Backwards-compatibility: Don't change this value.
			//Shields originally were indexed immediately after swords in the
			//same enumeration. This is the offset value for transforming the
			//original shield values into the current values.
};

enum AccessoryType
{
	NoAccessory=0,
	GrapplingHook=1,
	WaterBoots=2,
	InvisibilityPotion=3,
	SpeedPotion=4,
	HandBomb=5,
	PickAxe=6,
	WarpToken=7,
	PortableOrb=8,
	LuckyGold=9,
	WallWalking=10,
	XPDoubler=11,
	AccessorySlot=12,
	AccessoryCount,

	AccessoryShieldOffset=5 //Backwards-compatibility: Don't change this value.
			//Accessories originally were indexed immediately after shields in the
			//same enumeration. This is the offset value for transforming the
			//original accessory values into the current values.
};

static inline bool bIsValidStandardWeapon(const UINT t){return t<WeaponSlot;}
static inline bool bIsValidStandardShield(const UINT t){return t<ArmorSlot;}
static inline bool bIsValidStandardAccessory(const UINT t){return t<AccessorySlot;}

//******************************************************************************************
//Environmental weather conditions.
//Currently, this is completely aesthetic, not affecting game logic in any way.
struct Weather
{
	void clear() {
		bOutside = bLightning = bClouds = bSunshine = bSkipLightfade = false;
		wFog = wLight = wSnow = 0;
		sky.resize(0);
	}
	bool bOutside;		//area is located outside
	bool bLightning;	//lightning flashes
	bool bClouds;     //overhead clouds are shown
	bool bSunshine;   //shadows from overhead clouds are cast onto ground
	bool bSkipLightfade;  //skip room light crossfade on room transition
	UINT wFog;        //mist/fog rolls through room
	UINT wLight;      //room's ambient light level
	UINT wSnow;       //rate of snowfall
	WSTRING sky;      //name of sky image (if non-default)
};

//******************************************************************************************
class CCharacter;
class CDbHold;
class CDbLevel;
class CDbRooms;
class CDbSavedGame;
class CDbSpeech;
class CDbDatum;
class CCueEvents;
class CPlayerDouble;
class CPlatform;
struct ExploredRoom;
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
	CDbRoom(const CDbRoom &Src);
	CDbRoom &operator= (const CDbRoom &Src) {
		SetMembers(Src);
		return *this;
	}
	void MakeCopy(const CDbRoom &Src) {SetMembers(Src, false);}
	CDbRoom* MakeCopy(CImportInfo& info, const UINT newHoldID) const;

	virtual ~CDbRoom();

	UINT          dwRoomID;
	UINT          dwLevelID;
	UINT          dwRoomX;
	UINT          dwRoomY;
	UINT           wRoomCols;
	UINT           wRoomRows;
	WSTRING        style;
//	bool           bIsRequired;   //room must be conquered to open blue doors
	bool           bIsSecret;     //room is marked as a secret
	UINT          dwDataID;      //for special floor image mosaic
	UINT           wImageStartX, wImageStartY;   //where mosaic starts tiling
	char *            pszOSquares;   //currently supports up to 256 tile types
	char *            pszFSquares;
	char *            pszTSquares;
	UINT *            pszTParams;
	vector<COrbData*> orbs;
	vector<CScrollData*> Scrolls;
	vector<CExitData*>   Exits;
//	CCoordSet      halphEnters, slayerEnters;   //where Halph/Slayer can enter the room
//	CCoordSet      checkpoints;
	CDbPackedVars  ExtraVars;
	CMonster       *pFirstMonster, *pLastMonster;
	CCoordIndex_T<short> tileLights;       //per-tile lighting

	//In-game state information.
	CMonster **    pMonsterSquares;  //points to monster occupying each square
//	UINT           wMonsterCount;
//	UINT           wBrainCount;
	UINT           wTarLeft;
	UINT           wTrapDoorsLeft;
	bool           bBetterVision;    //sight token
//	bool           bPersistentCitizenMovement; //citizen movement algorithm
//	CMonster      *pLastClone;       //last clone queried
	UINT           mapMarker;


//	CPathMap *     pPathMap[NumMovementTypes];
	CCoordSet      LitFuses, NewFuses;  //all fuse pieces (and bombs) burning this turn
	CCoordStack    NewBabies; //unstable tar tiles marked for baby conversion
	CBriars        briars;   //briar sources in the room
//	list<CPlayerDouble*> Decoys, stalwarts;  //player decoys, stalwarts in the room
	vector<CPlatform*>   platforms;  //all moving platforms in the room
//	vector<CStation*>    stations;   //all relay stations in the room
	CCoordIndex    coveredOSquares;  //what is under removable o-tile objects
	CCoordIndex    coveredTSquares;  //what is under movable t-tile objects
	CCoordIndex_T<USHORT> pressurePlateIndex; //which pressure plate is on this square
	bool				bTarWasStabbed;	//for "dangerous room" heuristic
//	bool				bGreenDoorsOpened;	//when all monsters have been killed
	Weather        weather;	//environmental weather conditions
	CBridge        bridges;
//	CBuilding      building; //tiles marked for building

	CCoordSet      geometryChanges, disabledLights; //for front end -- where lighting must be updated

	//Uniform way of accessing 2D information in 1D array (column-major).
	inline UINT    ARRAYINDEX(const UINT x, const UINT y) const {return (y * this->wRoomCols) + x;}

	void           ActivateOrb(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const OrbActivationType eActivationType);
	void           ActivateToken(CCueEvents &CueEvents, const UINT wX, const UINT wY);
//	void           AddDiagonalDoorAssociations();
	CMonster *     AddNewMonster(const UINT wMonsterType, const UINT wX,
			const UINT wY, const bool bInRoom=true);
	bool           AddOrb(COrbData *pOrb);
	COrbData *     AddOrbToSquare(const UINT wX, const UINT wY);
	bool           AddPressurePlateTiles(COrbData* pPlate);
	bool           AddScroll(CScrollData *pScroll);
	bool           AddExit(CExitData *pExit);
	void           BombExplode(CCueEvents &CueEvents, CCoordStack& bombs);
	void           BurnFuses(CCueEvents &CueEvents);
	void           BurnFuseEvents(CCueEvents &CueEvents);
//	bool           BrainSensesSwordsman() const;
//	bool           BrainSensesTarget() const;
	bool           CanMovePlatform(const UINT wX, const UINT wY, const UINT wO);
	bool           CanSetSwordsman(const UINT dwX, const UINT dwY) const;
//			const bool bRoomConquered=true) const;
	inline UINT    CalcRoomArea() const {return this->wRoomCols * this->wRoomRows;}
	bool           CanPlayerMoveOnThisElement(const UINT wAppearance,
			const UINT wTileNo, const bool bRaisedSrc) const;
	bool           CanPushTo(const UINT wFromX, const UINT wFromY,
			const UINT wX, const UINT wY) const;
	bool           CanJumpTo(const UINT wFromX, const UINT wFromY,
			const UINT wX, const UINT wY, const bool bFromRaisedTile) const;
	void           ChangeTiles(const RoomTokenType tType);
	void           CharactersCheckForCueEvents(CCueEvents &CueEvents, CMonster* pMonsterList);
	void           CheckForFallingAt(const UINT wX, const UINT wY, CCueEvents& CueEvents, const bool bTrapdoorFell=false);
	void           ClearDeadMonsters();
	void           ClearMonsters();
	void           ClearPlatforms();
/*
	void           CreatePathMap(const UINT wX, const UINT wY,
			const MovementType eMovement);
	void           CreatePathMaps();
*/
	bool           CropRegion(UINT& x1, UINT &y1, UINT &x2, UINT &y2) const;

	int            DangerLevel() const;
//	void           DecMonsterCount();
	void           DecTrapdoor(CCueEvents &CueEvents);
	void           DeleteExitAtSquare(const UINT wX, const UINT wY);
	void           DeleteOrbAtSquare(const UINT wX, const UINT wY);
	void           DeleteScrollTextAtSquare(const UINT wX, const UINT wY);
	void           DestroyCrumblyWall(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const UINT wStabO=NO_ORIENTATION);
	void           DestroyTar(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           DestroyTrapdoor(const UINT wX, const UINT wY, CCueEvents &CueEvents);
/*	bool           DoesMonsterEnterRoomLater(const UINT wX, const UINT wY,
			const UINT wMonsterType) const;
	bool           DoesSquareContainDoublePlacementObstacle(const UINT wX, const UINT wY) const;
	bool           DoesSquareContainPathMapObstacle(const UINT wX, const UINT wY,
			const MovementType eMovement) const;
*/
	bool           DoesSquareContainPlayerObstacle(const UINT wX, const UINT wY,
			const UINT wO, const bool bRaisedSrc) const;
	bool           DoesSquarePreventDiagonal(const UINT wX, const UINT wY,
			const int dx, const int dy) const;
	void           ExpandBriars(CCueEvents& CueEvents);
//	CMonster*      FindNextClone();
	void           FixUnstableTar(CCueEvents& CueEvents);
	void           FindOrbsToOpenDoor(CCoordSet& orbs, CCoordSet& doorSquares) const;
	void           FloodPlot(const UINT wX, const UINT wY, const UINT wNewTile,
			const bool b8Neighbor=true);
	UINT           FuseEndAt(const UINT wCol, const UINT wRow, const bool bLighting=true) const;
	void           GetAllDoorSquares(const UINT wX, const UINT wY, CCoordSet& squares,
			const UINT tile, const CCoordSet* pIgnoreSquares=NULL) const;
	CCharacter*    GetCharacterWithScriptID(const UINT scriptID);
	void           GetConnectedRegionsAround(const UINT wX, const UINT wY,
			const CTileMask &tileMask, vector<CCoordSet>& regions,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const;
	void           GetConnectedTiles(const UINT wX, const UINT wY,
			const CTileMask &tileMask, const bool b8Neighbor, CCoordSet& squares,
			const CCoordSet* pIgnoreSquares=NULL, const CCoordSet* pRegionMask=NULL) const;
	CCurrentGame*  GetCurrentGame() const {return this->pCurrentGame;}
	UINT           GetExitIndexAt(const UINT wX, const UINT wY) const;
	bool           GetExitEntranceIDAt(const UINT wX, const UINT wY, UINT &dwEntranceID) const;
	UINT          GetImportCharacterSpeechID();
	CDbLevel*      GetLevel() const;
	void           GetLevelPositionDescription(WSTRING &wstrDescription,
			const bool bAbbreviate=false);
	void           GetDoubleSwordCoords(CCoordIndex &DoubleSwordCoords,
			CMonster *pIgnore=NULL) const;
	UINT           GetFSquare(const UINT wX, const UINT wY) const;
	CMonster*      GetMonsterAtSquare(const UINT wX, const UINT wY) const;
	CMonster*      GetMonsterOfType(const UINT wType) const;
	UINT           GetMonsterTypeAt(const UINT wX, const UINT wY,
			const bool bConsiderNPCIdentity=false, const bool bOnlyLiveMonsters=true) const;
	CMonster*      GetNPCBeethro() const;
	COrbData*      GetOrbAtCoords(const UINT wX, const UINT wY) const;
	UINT           GetOSquare(const UINT wX, const UINT wY) const;
	UINT           GetOSquareWithGuessing(const int nX, const int nY) const;
	COrbData*      GetPressurePlateAtCoords(const UINT wX, const UINT wY) const;
	UINT           GetPrimaryKey() const {return this->dwRoomID;}
	void           getStats(RoomStats& stats, const CDbLevel *pLevel) const;
	const WCHAR*   GetScrollTextAtSquare(const UINT wX, const UINT wY) const;
	CScrollData*   GetScrollAtSquare(const UINT wX, const UINT wY) const;
	CEntity*       GetSpeaker(const UINT wType, const bool bConsiderBaseType=false);
	void           GetSwordCoords(CCoordIndex &SwordCoords, CMonster *pIgnore=NULL) const;
	CPlatform*     GetPlatformAt(const UINT wX, const UINT wY) const;
	void           GetTarConnectedComponent(const UINT wX, const UINT wY,
			CCoordSet& tiles, const bool bAddAdjOnly=false) const;
	UINT           GetTSquare(const UINT wX, const UINT wY) const;
	UINT           GetTParam(const UINT wX, const UINT wY) const;
	UINT           GetTSquareWithGuessing(const int nX, const int nY) const;
	bool           HasClosedDoors() const;
	bool           HasCombatableMonsters() const;
	bool           HasGrabbableItems() const;

	void           IncTrapdoor(CCueEvents& CueEvents);
	void           InitCoveredTiles();
	void           InitRoomStats(const bool bSkipPlatformInit=false);
	UINT           GetBrainsPresent() const;
	bool           IsDisarmTokenActive() const;
	bool           IsDoorOpen(const int nCol, const int nRow);
	bool           IsOrbBeingStruck(const UINT wX, const UINT wY) const;
	bool           IsMonsterInRect(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const bool bConsiderPieces=true) const;
	bool           IsMonsterInRectOfType(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const UINT wType,
			const bool bConsiderNPCIdentity=false) const;
	bool           IsMonsterNextTo(const UINT wX, const UINT wY, const UINT wType) const;
	bool           IsMonsterOfTypeAt(const UINT eType, const UINT wX, const UINT wY,
			const bool bConsiderNPCIdentity=false, const bool bOnlyLiveMonsters=true) const;
	bool           IsMonsterWithin(const UINT wX, const UINT wY,
			const UINT wSquares, const bool bConsiderPieces=true) const;
	bool           IsMonsterSwordAt(const UINT wX, const UINT wY,
			const CMonster *pIgnore=NULL) const;
/*
	bool           IsPathmapNeeded() const;
	bool           IsTimerNeeded() const;
	static bool    IsRequired(const UINT dwRoomID);
*/
	static bool    IsSecret(const UINT dwRoomID);
	bool           IsSwordAt(const UINT wX, const UINT wY) const;

	bool           IsSwordWithinRect(const UINT wMinX, const UINT wMinY,
			const UINT wMaxX, const UINT wMaxY) const;
	bool           IsTarStableAt(const UINT wX, const UINT wY, const UINT wTarType) const;
	bool           IsTarVulnerableToStab(const UINT wX, const UINT wY) const;
	CMonster*      GetMotherConnectedToTarTile(const UINT wX, const UINT wY) const;
	bool           IsTileInRectOfType(const UINT wLeft, const UINT wTop,
			const UINT wRight, const UINT wBottom, const UINT wType) const;
	bool           IsValidColRow(const UINT wX, const UINT wY) const;
	void           KillSeepOutsideWall(CCueEvents &CueEvents);
	bool           KillMonster(CMonster *pMonster, CCueEvents &CueEvents, const bool bForce=false);
	bool           KillMonsterAtSquare(const UINT wX, const UINT wY,
			CCueEvents &CueEvents, const bool bForce=false);
	void           LightFuse(CCueEvents &CueEvents, const UINT wCol, const UINT wRow,
			const bool bDelayProcessing=true);
	void           LinkMonster(CMonster *pMonster, const bool bInRoom=true);
	bool           Load(const UINT dwLoadRoomID, const bool bQuick=false);
	CMonster*      LoadMonster(const c4_RowRef& row);
	bool           LoadMonstersLate();
	bool           LoadTiles();
	void           MarkSpeechForDeletion(CDbSpeech* pSpeech);
	void           MarkDataForDeletion(const CDbDatum* pDatum);
	bool           MonsterHeadIsAt(const UINT wX, const UINT wY) const;
	CMonster*      MonsterOfTypeExists(const UINT eType, const bool bConsiderNPCIdentity=false) const;
//	bool           MonsterWithMovementTypeExists(const MovementType eMovement) const;
	void           MoveMonster(CMonster* const pMonster, const UINT wDestX, const UINT wDestY);
	void           MovePlatform(const UINT wX, const UINT wY, const UINT wO);
	void           MoveScroll(const UINT wX, const UINT wY,
			const UINT wNewX, const UINT wNewY);
	void           PlaceCharacters(CDbHold *pHold=NULL);
	void           Plot(const UINT wX, const UINT wY, const UINT wTileNo,
			CMonster *pMonster=NULL);
	void           Plot(const CCoordSet& plots, const bool bChangesRoomGeometry=false);
	void           PreprocessMonsters(CCueEvents& CueEvents);
	static bool    PressurePlateIsDepressedBy(const UINT item);
	void           ProcessTurn(CCueEvents &CueEvents, const bool bFullMove);
	void           ExpandExplosion(CCueEvents &CueEvents, CCoordStack& cs,
			const UINT wBombX, const UINT wBombY, const UINT wX, const UINT wY,
			CCoordStack& bombs, CCoordSet& explosion);
	void           ProcessExplosionSquare(CCueEvents &CueEvents, const UINT wX, const UINT wY,
			const bool bHarmsPlayer=true);
	void           ProcessAumtlichGaze(CCueEvents &CueEvents);
	void           PushObject(const UINT wSrcX, const UINT wSrcY,
			const UINT wDestX, const UINT wDestY, CCueEvents& CueEvents);
//	void           RecalcStationPaths();
	void           ReflectX();
	void           ReflectY();
	void           Reload();
//	void           RemoveFinishedCharacters();
	bool           RemovePressurePlateTile(const UINT wX, const UINT wY);
	void           RemoveStabbedTar(const UINT wX, const UINT wY, CCueEvents &CueEvents);
	void           ConvertUnstableTar(CCueEvents &CueEvents, const bool bDelayBabyMoves=false);
	bool           RemoveTiles(const UINT wOldTile);
	void           RemoveDoorTile(const UINT wX, const UINT wY,
			const UINT wTile);
	void           ResetExitIDs();
	static void    ResetForImport();
	void           ResetMonsterFirstTurnFlags();
	void           ResetPressurePlatesState();
	void           ResetTurnFlags();
	void           RotateClockwise();

	void           SetCurrentGame(CCurrentGame *const pSetCurrentGame);
	void           SetHoldForMonsters(CDbHold* pHold); //back door
	void           SetMembersFromExploredRoomData(ExploredRoom *pExpRoom);
	void           SetMonstersFromExploredRoomData(ExploredRoom* pExpRoom,
			const bool bLoadNPCScripts);

//	void           SetHalphSlayerEntrance();
	void           SetMonsterSquare(CMonster *pMonster);
//	void           SetPathMapsTarget(const UINT wX, const UINT wY);
	void           SetPressurePlatesState();
	void           SetSwordsSheathed();
	void           SetTParam(const UINT wX, const UINT wY, const UINT value);

	//Import handling
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);

	void           SetScrollTextAtSquare(const UINT wX, const UINT wY, const WCHAR* pwczScrollText);
	void           SetExit(const UINT dwEntranceID, const UINT wX, const UINT wY,
			const UINT wX2=(UINT)-1, const UINT wY2=(UINT)-1);
	bool           SomeMonsterCanSmellSwordsman() const;
	bool           StabTar(const UINT wX, const UINT wY, CCueEvents &CueEvents,
			const bool removeTarNow, const UINT wStabO=NO_ORIENTATION);
	void           SwitchTarstuff(const UINT wType1, const UINT wType2);
	bool           SwordfightCheck() const;
	void           ToggleBlackGates(CCueEvents& CueEvents);
	void           ToggleDoor(const UINT wX, const UINT wY);
	bool           ToggleTiles(const UINT wOldTile, const UINT wNewTile);
	void           ToggleLight(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	bool     TunnelGetExit(const UINT wStartX, const UINT wStartY,
		const int dx, const int dy, UINT& wX, UINT& wY) const;
	void           TurnOffLight(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	void           TurnOnLight(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	void           UnlinkMonster(CMonster *pMonster);
	bool           UnpackSquares(const BYTE *pSrc, const UINT dwSrcSize);
	virtual bool   Update();
//	void           UpdatePathMapAt(const UINT wX, const UINT wY);

private:
	enum tartype {oldtar, newtar, notar};

	void           AddPlatformPiece(const UINT wX, const UINT wY, CCoordIndex &plots);
	void           Clear();
	void           CloseDoor(const UINT wX, const UINT wY);
//	void           DeletePathMaps();
	CMonster*      FindLongMonster(const UINT wX, const UINT wY,
			const UINT wFromDirection=10) const;
	void           GetLevelPositionDescription_English(WSTRING &wstrDescription,
			const int dx, const int dy, const bool bAbbrev=false);
	UINT           GetLocalID() const;
	void           GetNumber_English(const UINT num, WCHAR *str);
	bool           LoadOrbs(c4_View &OrbsView);
	bool           LoadMonsters(c4_View &MonstersView);
	bool           LoadScrolls(c4_View &ScrollsView);
	bool           LoadExits(c4_View &ExitsView);
	void           MarkTilesForStyle(const CTileMask &mask,
			UINT* const tileTypes, const UINT numTileTypes, const UINT wMajorityStyle);
	void           MarkTilesFromSquare(const UINT wX, const UINT wY,
			const CTileMask &mask, UINT* const tileTypes, const UINT numTileTypes,
			const UINT wMajorityStyle, const bool b8Neighbor);
	bool           NewGelWouldBeStable(const vector<tartype> &addedGel, const UINT tx, const UINT ty, const CCoordSet& contiguousGel);
	bool           NewTarWouldBeStable(const vector<tartype> &addedTar, const UINT tx, const UINT ty);
	void           ObstacleFill(CCoordIndex& obstacles);
	void           OpenDoor(const UINT wX, const UINT wY);
	c4_Bytes *     PackSquares() const;
	c4_Bytes *     PackTileLights() const;
	void           ReevalBriarNear(const UINT wX, const UINT wY, const UINT wTileNo);
	void           ReflectSquare(const bool bHoriz, UINT &wSquare) const;
//	bool           RemoveLongMonsterPieces(CMonster *pMonster);
	void           RotateTileC(UINT &wTile) const;
	void           SaveOrbs(c4_View &OrbsView) const;
	void           SaveMonsters(c4_View &MonstersView);
	void           SaveScrolls(c4_View &ScrollsView);
	void           SaveExits(c4_View &ExitsView) const;
	void           SetCurrentGameForMonsters(const CCurrentGame *pSetCurrentGame);
	void           SetExtraVarsFromMembers();
	bool           SetMembers(const CDbRoom &Src, const bool bCopyLocalInfo=true);
	void           SetMembersFromExtraVars();

	bool           UnpackTileLights(const BYTE *pSrc, const UINT dwSrcSize);

	bool           UpdateExisting();
	bool           UpdateNew();

	list<CMonster *>  DeadMonsters;
	vector<UINT> deletedScrollIDs;  //message text IDs to be deleted on Update
	vector<UINT> deletedSpeechIDs;  //speech IDs to be deleted on Update
	vector<UINT> deletedDataIDs;    //data IDs to be deleted on Update
	CCurrentGame * pCurrentGame;
//	bool           bCheckForHoldMastery;
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
	virtual void   ExportXML(const UINT dwRoomID, CDbRefs &dbRefs, string &str, const bool bRef=false);
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
