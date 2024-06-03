// $Id: PlayerStats.h 9103 2008-07-14 08:31:18Z trick $

#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include "DbPackedVars.h"
#include <BackEndLib/Wchar.h>
#include <string>
using std::string;

//Script variable manipulation.
//
//To maintain backwards compatibility, don't alter the enumeration values.
namespace ScriptVars
{
	enum Op
	{
		Assign=0,
		Inc=1,
		Dec=2,
		AssignText=3,
		AppendText=4,
		MultiplyBy=5,
		DivideBy=6,
		Mod=7
	};
	enum Comp
	{
		Equals=0,
		Greater=1,
		Less=2,
		EqualsText=3,
		LessThanOrEqual = 8, //to avoid collision with Op values
		GreaterThanOrEqual = 9,
		Inequal = 10
	};

	enum StatModifiers
	{
		MonsterHP = 0,
		MonsterATK = 1,
		MonsterDEF = 2,
		MonsterGR = 3,
		MonsterXP = 4,
		ItemAll = 5,
		ItemHP = 6,
		ItemATK = 7,
		ItemDEF = 8,
		ItemGR = 9,
		ItemShovels = 10
	};

	//Predefined global and relative game state vars, accessed through these key values.
	//DO NOT CHANGE
	enum Predefined
	{
		P_NoVar  =  0,
		P_HP     = -1,
		P_ATK    = -2,
		P_DEF    = -3,
		P_GOLD   = -4,
		P_YKEY   = -5,
		P_GKEY   = -6,
		P_BKEY   = -7,
		P_SWORD  = -8,
		P_SHIELD = -9,
		P_SPEED  = -10,
		P_MONSTER_HP  = -11,
		P_MONSTER_ATK = -12,
		P_MONSTER_DEF = -13,
		P_MONSTER_GOLD = -14,
		P_MONSTER_COLOR = -15,
		P_XP = -16,
		P_MONSTER_HP_MULT = -17,
		P_MONSTER_ATK_MULT = -18,
		P_MONSTER_DEF_MULT = -19,
		P_MONSTER_GOLD_MULT = -20,
		P_ITEM_MULT = -21,
		P_MONSTER_SWORD = -22,
		P_SKEY = -23,
		P_PLAYER_X = -24,
		P_PLAYER_Y = -25,
		P_PLAYER_O = -26,
		P_MONSTER_X = -27,
		P_MONSTER_Y = -28,
		P_MONSTER_O = -29,
		P_ACCESSORY = -30,
		P_MONSTER_XP = -31,
		P_MONSTER_XP_MULT = -32,
		P_ITEM_HP_MULT = -33,
		P_ITEM_ATK_MULT = -34,
		P_ITEM_DEF_MULT = -35,
		P_ITEM_GR_MULT = -36,
		P_TOTALMOVES = -37,
		P_TOTALTIME = -38,
		P_SCRIPT_X = -39,
		P_SCRIPT_Y = -40,
		P_SCRIPT_W = -41,
		P_SCRIPT_H = -42,
		P_SCRIPT_F = -43,
		P_HOTTILE = -44,
		P_EXPLOSION = -45,
		P_PRIOR_ROOMID = -46,
		P_PRIOR_X = -47,
		P_PRIOR_Y = -48,
		P_PRIOR_O = -49,
		P_ENEMY_HP = -50,
		P_ENEMY_ATK = -51,
		P_ENEMY_DEF = -52,
		P_ENEMY_GOLD = -53,
		P_ENEMY_XP = -54,
		P_WEAPON_ATK = -55,
		P_WEAPON_DEF = -56,
		P_WEAPON_GR = -57,
		P_ARMOR_ATK = -58,
		P_ARMOR_DEF = -59,
		P_ARMOR_GR = -60,
		P_ACCESSORY_ATK = -61,
		P_ACCESSORY_DEF = -62,
		P_ACCESSORY_GR = -63,
		P_SCRIPT_MONSTER_HP_MULT = -64,
		P_SCRIPT_MONSTER_ATK_MULT = -65,
		P_SCRIPT_MONSTER_DEF_MULT = -66,
		P_SCRIPT_MONSTER_GOLD_MULT = -67,
		P_SCRIPT_MONSTER_XP_MULT = -68,
		P_SCRIPT_ITEM_MULT = -69,
		P_SCRIPT_ITEM_HP_MULT = -70,
		P_SCRIPT_ITEM_ATK_MULT = -71,
		P_SCRIPT_ITEM_DEF_MULT = -72,
		P_SCRIPT_ITEM_GR_MULT = -73,
		P_MUD_SPAWN = -74,
		P_TAR_SPAWN = -75,
		P_GEL_SPAWN = -76,
		P_QUEEN_SPAWN = -77,
		P_MONSTER_NAME = -78,
		P_SCRIPT_MONSTER_SPAWN = - 79,
		P_SCORE_HP = -80,
		P_SCORE_ATK = -81,
		P_SCORE_DEF = -82,
		P_SCORE_YKEY = -83,
		P_SCORE_GKEY = -84,
		P_SCORE_BKEY = -85,
		P_SCORE_SKEY = -86,
		P_SCORE_GOLD = -87,
		P_SCORE_XP = -88,
		P_MONSTER_CUSTOM_WEAKNESS = -89,
		P_LEVEL_MULT = -90,
		P_ROOM_X = -91,
		P_ROOM_Y = -92,
		P_MONSTER_CUSTOM_DESCRIPTION = -93,
		P_SHOVEL = -94,
		P_SCORE_SHOVEL = -95,
		P_ITEM_SHOVEL_MULT = -96,
		P_SCRIPT_ITEM_SHOVEL_MULT = -97,
		P_BEAM = -98,
		P_FIRETRAP = -99,
		P_TOTAL_ATK = -100,
		P_TOTAL_DEF = -101,
		FirstPredefinedVar = P_TOTAL_DEF, //set this to the last var in the enumeration
		PredefinedVarCount = -int(FirstPredefinedVar)
	};

	//Predefined functions (that take a set of arguments to calculate a value)
	//
	//To add new primitives, add a new enumeration here and in the following locations:
	// primitiveNames
	// getPrimitiveRequiredParameters
	// CCurrentGame::EvalPrimitive
	enum PrimitiveType {
		NoPrimitive = -1,
		P_Abs,       //x --> abs(x)
		P_Min,       //(x,y) --> min(x,y)
		P_Max,       //(x,y) --> max(x,y)
		P_Orient,    //(dx,dy) --> o
		P_OrientX,   //o --> dx
		P_OrientY,   //o --> dy
		P_RotateCW,  //o --> cw(o)
		P_RotateCCW, //o --> ccw(o)
		P_Dist0,     //L-infinity norm
		P_Dist1,     //L-1 norm (Manhattan distance)
		P_Dist2,     //L-2 norm (Euclidean distance)
		P_EnemyStat, //(x,y,stat) --> stat value of enemy at (x,y)
		PrimitiveCount
	};

	void init();
	UINT getVarDefault(const ScriptVars::Predefined var);
	string getVarName(const ScriptVars::Predefined var);
	WSTRING getVarNameW(const ScriptVars::Predefined var);
	bool IsStringVar(Predefined val);
	Predefined parsePredefinedVar(const string& str);
	Predefined parsePredefinedVar(const WSTRING& wstr);

	PrimitiveType parsePrimitive(const string& str);
	UINT getPrimitiveRequiredParameters(PrimitiveType eType);
	PrimitiveType parsePrimitive(const WSTRING& wstr);

	//All predefined vars.
	extern const char predefinedVarTexts[PredefinedVarCount][16];
	extern const UINT predefinedVarMIDs[PredefinedVarCount];
	extern string midTexts[PredefinedVarCount];
	extern const char primitiveNames[PrimitiveCount][11]; //expand buffer size as needed

	//Global game var subset quick reference.
	static const UINT numGlobals=44;
	extern const Predefined globals[numGlobals];
	extern const UINT globalVarMIDs[numGlobals];
	extern const char* globalVarShortNames[numGlobals];
};

//RPG stats.
class PlayerStats
{
public:
	PlayerStats() {clear();}
	virtual ~PlayerStats() {}

	virtual void clear() {
		HP = 0;
		ATK = DEF = 0;
		GOLD = XP = 0;
		speed = 0;
		yellowKeys = greenKeys = blueKeys = skeletonKeys = 0;
		shovels = 0;
		sword = shield = accessory = 0;
		monsterHPmult = monsterATKmult = monsterDEFmult = monsterGRmult = monsterXPmult = 0;
		itemMult = itemHPmult = itemATKmult = itemDEFmult = itemGRmult = itemShovelMult = 0;
		hotTileVal = explosionVal = beamVal = firetrapVal = 0;
		totalMoves = totalTime = 0;
		priorRoomID = priorX = priorY = priorO = 0;
		mudSpawnID = tarSpawnID = gelSpawnID = queenSpawnID = UINT(-1); //negative indicates default
		scoreHP = scoreATK = scoreDEF = scoreYellowKeys = scoreGreenKeys = scoreBlueKeys = scoreSkeletonKeys = scoreGOLD = scoreXP = scoreShovels = 0;
	}

	UINT getVar(const WSTRING& wstr) const
	{
		return getVar(ScriptVars::parsePredefinedVar(wstr));
	}
	UINT getVar(const ScriptVars::Predefined var) const;
	void setVar(const ScriptVars::Predefined var, const UINT val);

	static bool IsGlobalStatIndex(UINT i);
	void Pack(CDbPackedVars& stats);
	void Unpack(CDbPackedVars& stats);

//protected:
	UINT HP;
	int  ATK, DEF;            //stats
	int  GOLD, XP;            //may be negative
	UINT speed;               //attack rate
	UINT yellowKeys, greenKeys, blueKeys, skeletonKeys;  //keys
	UINT shovels;
	UINT sword, shield, accessory;         //equipment
	UINT totalMoves, totalTime; //tally stats

	//Global modifiers
	UINT monsterHPmult, monsterATKmult, monsterDEFmult, monsterGRmult, monsterXPmult; //global monster stat modifiers
	UINT itemMult, itemHPmult, itemATKmult, itemDEFmult, itemGRmult, itemShovelMult; //global item value modifiers
	UINT hotTileVal, explosionVal, beamVal, firetrapVal; //damage modifiers

	//Prior location before level warp.
	UINT priorRoomID, priorX, priorY, priorO;

	int mudSpawnID, tarSpawnID, gelSpawnID, queenSpawnID;
	int scoreHP, scoreATK, scoreDEF, scoreYellowKeys, scoreGreenKeys, scoreBlueKeys, scoreSkeletonKeys, scoreGOLD, scoreXP, scoreShovels;
};

//More stats used for various tally operations.
class RoomStats : public PlayerStats
{
public:
	RoomStats() : PlayerStats() {clear();}
	virtual ~RoomStats() {}

	virtual void clear() {
		PlayerStats::clear();
		yellowDoors = greenDoors = blueDoors = redDoors = blackDoors = 0;
		openYellowDoors = openGreenDoors = openBlueDoors = openRedDoors = openBlackDoors = 0;
		moneyDoorCost = openMoneyDoorCost = 0;
		dirtBlockCost = 0;
		rooms = secrets = levels = 0;
	}

	UINT yellowDoors, greenDoors, blueDoors, redDoors, blackDoors;
	UINT openYellowDoors, openGreenDoors, openBlueDoors, openRedDoors, openBlackDoors;
	UINT moneyDoorCost, openMoneyDoorCost;
	UINT dirtBlockCost;
	UINT rooms, secrets, levels;
};

#endif //PLAYERSTATS_H
