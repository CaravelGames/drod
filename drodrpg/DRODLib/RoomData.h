// $Id$

#ifndef ROOMDATA_H
#define ROOMDATA_H

#include "TileConstants.h"
#include "DbMessageText.h"
#include "HoldRecords.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <vector>

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
{
	return action > OA_NULL && action < OA_COUNT;
}
static inline bool bIsValidOrbType(const OrbType type)
{
	return type >= OT_NORMAL && type < OT_COUNT;
}

static inline OrbAgentType operator--(OrbAgentType& d) { return d = static_cast<OrbAgentType>(d - 1); }
static inline OrbAgentType operator--(OrbAgentType& d, int) { OrbAgentType const t = d; --d; return t; }
static inline OrbAgentType operator++(OrbAgentType& d) { return d = static_cast<OrbAgentType>(d + 1); }
static inline OrbAgentType operator++(OrbAgentType& d, int) { OrbAgentType const t = d; ++d; return t; }

class COrbAgentData : public CCoord
{
public:
	COrbAgentData() : CCoord(0, 0), action(OA_NULL)
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
		: CCoord(0, 0), eType(OT_NORMAL), bActive(false)
	{ }
	~COrbData()
	{
		ClearAgents();
	}

	COrbData(const UINT wX, const UINT wY)
		: CCoord(wX, wY), eType(OT_NORMAL), bActive(false) { }
	COrbData(const COrbData& Src) : CCoord() { SetMembers(Src); }
	COrbData& operator= (const COrbData& Src)
	{
		SetMembers(Src);
		return *this;
	}
	void ClearAgents()
	{
		UINT wIndex = this->agents.size();
		while (wIndex--)
			delete this->agents[wIndex];
		this->agents.clear();
	}

	COrbAgentData* AddAgent(const UINT wX, const UINT wY, const OrbAgentType action)
	{
		ASSERT(bIsValidOrbAgentType(action));
		COrbAgentData* pNewAgent = new COrbAgentData(wX, wY, action);
		this->agents.push_back(pNewAgent);
		return pNewAgent;
	}

	void AddAgent(COrbAgentData* pAgent)
	{
		this->agents.push_back(pAgent);
	}

	bool DeleteAgent(COrbAgentData* const pOrbAgent)
	{
		ASSERT(pOrbAgent);
		UINT wAgentI;
		for (wAgentI = this->agents.size(); wAgentI--; )
			if (pOrbAgent == this->agents[wAgentI])
			{
				//Found it.  Remove agent (replace with last one).
				delete this->agents[wAgentI];
				this->agents[wAgentI] = this->agents[this->agents.size() - 1];
				this->agents.pop_back();
				return true;
			}
		return false;  //didn't find it
	}

	COrbAgentData* GetAgentAt(const UINT wX, const UINT wY) const
	{
		for (UINT wAgentI = 0; wAgentI < this->agents.size(); ++wAgentI)
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
	void SetMembers(const COrbData& Src)
	{
		ClearAgents();
		this->wX = Src.wX;
		this->wY = Src.wY;
		this->eType = Src.eType;
		this->bActive = Src.bActive;
		this->tiles = Src.tiles;
		for (UINT wAgentI = 0; wAgentI < Src.agents.size(); ++wAgentI)
			this->agents.push_back(new COrbAgentData(Src.agents[wAgentI]));
	}
};

//******************************************************************************************
class CScrollData : public CCoord
{
public:
	CScrollData() : CCoord(0, 0)
	{ }

	CScrollData(CScrollData& Src) : CCoord() { SetMembers(Src); }
	CScrollData& operator= (CScrollData& Src)
	{
		SetMembers(Src);
		return *this;
	}

	CDbMessageText ScrollText;

private:
	void SetMembers(CScrollData& Src)
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
		: dwEntranceID(0), wLeft(0), wRight(0), wTop(0), wBottom(0), exitType(ExitType::ET_Entrance)
	{ }
	CExitData(const UINT dwEntranceID, const UINT wLeft, const UINT wRight,
		const UINT wTop, const UINT wBottom, const ExitType exitType = ExitType::ET_Entrance)
		: dwEntranceID(dwEntranceID), wLeft(wLeft), wRight(wRight), wTop(wTop), wBottom(wBottom)
		, exitType(exitType)
	{ }

	bool IsWorldMapExit() const { return exitType == ExitType::ET_WorldMap; }

	UINT       dwEntranceID;
	UINT        wLeft, wRight, wTop, wBottom;
	ExitType   exitType;
};

//******************************************************************************************
//Types of room tokens.
enum RoomTokenType
{
	RotateArrowsCW = 0,
	RotateArrowsCCW = 1,
	SwitchTarMud = 2,
	SwitchTarGel = 3,
	SwitchGelMud = 4,
	TarTranslucent = 5,
	SwordDisarm = 6,
	//	PowerTarget=7,
	//	PersistentCitizenMovement=8,
	RoomTokenCount
};

//Types of door keys.
enum KeyType
{
	YellowKey = 0,
	GreenKey = 1,
	BlueKey = 2,
	SkeletonKey = 3,
	KeyCount
};

//Types of swords.
enum SwordType
{
	NoSword = 0,
	WoodenBlade = 1,
	ShortSword = 2,
	GoblinSword = 3,
	LongSword = 4,
	HookSword = 5,
	ReallyBigSword = 6,
	LuckySword = 7,
	SerpentSword = 8,
	BriarSword = 9,
	WeaponSlot = 10,  //allows placing the held weapon at this location
	Dagger = 11,
	Staff = 12,
	Spear = 13,
	SwordCount
};

enum ShieldType
{
	NoShield = 0,
	WoodenShield = 1,
	BronzeShield = 2,
	SteelShield = 3,
	KiteShield = 4,
	OremiteShield = 5,
	ArmorSlot = 6,
	MirrorShield = 7,
	LeatherShield = 8,
	AluminumShield = 9,
	ShieldCount,
	ShieldSwordOffset = 8 //Backwards-compatibility: Don't change this value.
			//Shields originally were indexed immediately after swords in the
			//same enumeration. This is the offset value for transforming the
			//original shield values into the current values.
};

enum AccessoryType
{
	NoAccessory = 0,
	GrapplingHook = 1,
	WaterBoots = 2,
	InvisibilityPotion = 3,
	SpeedPotion = 4,
	HandBomb = 5,
	PickAxe = 6,
	WarpToken = 7,
	PortableOrb = 8,
	LuckyGold = 9,
	WallWalking = 10,
	XPDoubler = 11,
	AccessorySlot = 12,
	AccessoryCount,

	AccessoryShieldOffset = 5 //Backwards-compatibility: Don't change this value.
			//Accessories originally were indexed immediately after shields in the
			//same enumeration. This is the offset value for transforming the
			//original accessory values into the current values.
};

static inline bool bIsValidStandardWeapon(const UINT t) { return t < SwordCount && t != WeaponSlot; }
static inline bool bIsValidStandardShield(const UINT t) { return t < ShieldCount && t != ArmorSlot; }
static inline bool bIsValidStandardAccessory(const UINT t) { return t < AccessoryCount && t != AccessorySlot; }

//******************************************************************************************
//Environmental weather conditions.
//Currently, this is completely aesthetic, not affecting game logic in any way.
struct Weather
{
	void clear() {
		bOutside = bLightning = bClouds = bSunshine = bSkipLightfade = false;
		wFog = wLight = wSnow = rain = 0;
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
	UINT rain;        //rate of rainfall (0=none)
};

//******************************************************************************************
class RoomObject : public CCoord
{
public:
	RoomObject(UINT wX, UINT wY, UINT tile)
		: CCoord(wX, wY)
		, wPrevX(wX), wPrevY(wY)
		, tile(tile)
		, param(noParam())
		, coveredTile(emptyTile())
	{ }
	RoomObject(const RoomObject& obj)
		: CCoord(obj.wX, obj.wY)
		, tile(obj.tile)
		, param(obj.param)
		, coveredTile(obj.coveredTile)
	{ }

	static UINT emptyTile() { return T_EMPTY; }
	static UINT noParam() { return 0; }

	void operator=(UINT newTile) {
		tile = newTile;

		param = noParam();
		coveredTile = emptyTile();
	}

	void set_pos(UINT newX, UINT newY) {
		wPrevX = wX = newX;
		wPrevY = wY = newY;
	}
	void move(UINT newX, UINT newY) {
		wPrevX = wX;
		wPrevY = wY;

		wX = newX;
		wY = newY;
	}
	void syncPrevPosition() {
		wPrevX = wX;
		wPrevY = wY;
	}

	void place_under(const UINT tile) {
		coveredTile = tile;
	}

	void remove_top() {
		tile = coveredTile; //anything underneath is revealed
		remove_covered();
	}
	void remove_covered() {
		coveredTile = emptyTile();
	}

	bool cover(RoomObject* obj) {
		if (!obj)
			return false;

		coveredTile = obj->tile;
		param = obj->param;
		ASSERT(obj->coveredTile == emptyTile());
		return true;
	}

	//If a covered object exists, remove it from this record
	//and create a record for it
	RoomObject* uncover() {
		if (coveredTile == emptyTile())
			return NULL;

		RoomObject* pCoveredObj = new RoomObject(*this);
		pCoveredObj->remove_top();

		remove_covered();

		return pCoveredObj;
	}

	/*
	UINT getBottomTile() const {
		//Since covered tiles are relatively rare, check the "actual" T-layer
		//first, and only check the "covered" layer if a movable object is on top.
		if (bIsTLayerCoveringItem(tile)) {
			if (coveredTile != emptyTile())
				return coveredTile;
		}

		return tile;
	}
	*/

	UINT wPrevX, wPrevY;

	UINT tile;
	UINT param;

	UINT coveredTile;
};

#endif
