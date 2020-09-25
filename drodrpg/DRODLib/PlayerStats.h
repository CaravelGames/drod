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

	//Predefined global and relative game state vars, accessed through these key values.
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
		FirstPredefinedVar = P_ACCESSORY_GR, //set this to the last var in the enumeration
		PredefinedVarCount = -int(FirstPredefinedVar)
	};

	void init();
	string getVarName(const ScriptVars::Predefined var);
	WSTRING getVarNameW(const ScriptVars::Predefined var);
	Predefined parsePredefinedVar(const string& str);
	Predefined parsePredefinedVar(const WSTRING& wstr);

	//All predefined vars.
	extern const char predefinedVarTexts[PredefinedVarCount][13];
	extern const UINT predefinedVarMIDs[PredefinedVarCount];
	extern string midTexts[PredefinedVarCount];

	//Global game var subset quick reference.
	static const UINT numGlobals=26;
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

	void Pack(CDbPackedVars& stats);
	void Unpack(CDbPackedVars& stats);

	virtual void clear() {
		HP = 0;
		ATK = DEF = 0;
		GOLD = XP = 0;
		speed = 0;
		yellowKeys = greenKeys = blueKeys = skeletonKeys = 0;
		sword = shield = accessory = 0;
		monsterHPmult = monsterATKmult = monsterDEFmult = monsterGRmult = monsterXPmult = 0;
		itemMult = itemHPmult = itemATKmult = itemDEFmult = itemGRmult = 0;
		hotTileVal = explosionVal = 0;
		totalMoves = totalTime = 0;
		priorRoomID = priorX = priorY = priorO = 0;
	}

	UINT getVar(const WSTRING& wstr) const
	{
		return getVar(ScriptVars::parsePredefinedVar(wstr));
	}
	UINT getVar(const ScriptVars::Predefined var) const;
	void setVar(const ScriptVars::Predefined var, const UINT val);

//protected:
	UINT HP;
	int  ATK, DEF;            //stats
	int  GOLD, XP;            //may be negative
	UINT speed;               //attack rate
	UINT yellowKeys, greenKeys, blueKeys, skeletonKeys;  //keys
	UINT sword, shield, accessory;         //equipment
	UINT totalMoves, totalTime; //tally stats

	//Global modifiers
	UINT monsterHPmult, monsterATKmult, monsterDEFmult, monsterGRmult, monsterXPmult; //global monster stat modifiers
	UINT itemMult, itemHPmult, itemATKmult, itemDEFmult, itemGRmult; //global item value modifiers
	UINT hotTileVal, explosionVal; //damage modifiers

	//Prior location before level warp.
	UINT priorRoomID, priorX, priorY, priorO;
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
		rooms = secrets = levels = 0;
	}

	UINT yellowDoors, greenDoors, blueDoors, redDoors, blackDoors;
	UINT openYellowDoors, openGreenDoors, openBlueDoors, openRedDoors, openBlackDoors;
	UINT moneyDoorCost, openMoneyDoorCost;
	UINT rooms, secrets, levels;
};

#endif //PLAYERSTATS_H
