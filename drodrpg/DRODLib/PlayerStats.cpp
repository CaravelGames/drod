// $Id: PlayerStats.cpp 8917 2008-04-24 03:01:02Z mrimer $

#include "PlayerStats.h"
#include "Db.h"
#include "../Texts/MIDs.h"

using namespace ScriptVars;

//*****************************************************************************
//Shorter forms of the user-visible natural language var names.
//Used as keys to pack and unpack the global game vars into a CDbPackedVars object
//  (see Pack/Unpack methods below).
//Add key names here when adding a new global game var that needs to be saved.
//DO NOT CHANGE these names
const char ScriptVars::predefinedVarTexts[PredefinedVarCount][13] =
{
	"_HP", "_ATK", "_DEF", "_GOLD",
	"_YKEY", "_GKEY", "_BKEY",
	"_SWORD", "_SHIELD", "_SPEED",
	"", "", "", "", "", //predefined local vars are unused here: "_MyHP", "_MyATK", "_MyDEF", "_MyGOLD", "_MyColor", etc.
	"_XP",
	"_MonHPMult", "_MonATKMult", "_MonDEFMult", "_MonGRMult",
	"_ItemMult",
	"",
	"_SKEY",
	"", "", "",
	"", "", "",
	"_ACCESSORY",
	"",
	"_MonXPMult",
	"_ItemHPMult", "_ItemATKMult", "_ItemDEFMult", "_ItemGRMult",
	"_TotalMoves", "_TotalTime",
	"", "", "", "", "",
	"_HotTile", "_Explosion",
	"_PRID", "_PX", "_PY", "_PO", //prior location before level entrance warp
	"", "", "", "", "",
	"", "", "",
	"", "", "",
	"", "", "",
	"", "", "", "", "",
	"", "", "", "", "",
	"_MudSpawn", "_TarSpawn", "_GelSpawn", "_QueenSpawn"
};

//Message texts corresponding to the above short var texts.
const UINT ScriptVars::predefinedVarMIDs[PredefinedVarCount] = {
	MID_VarHP, MID_VarAtk, MID_VarDef, MID_VarGold,
	MID_VarYKey, MID_VarGKey, MID_VarBKey,
	MID_VarSword, MID_VarShield, 0,
	MID_VarMonsterHP, MID_VarMonsterAtk, MID_VarMonsterDef, MID_VarMonsterGold, MID_VarMonsterColor,
	MID_VarXP,
	MID_VarMonsterHPMult, MID_VarMonsterATKMult, MID_VarMonsterDEFMult, MID_VarMonsterGRMult,
	MID_VarItemMult, MID_VarMonsterSword, MID_VarSKey,
	MID_VarX, MID_VarY, MID_VarO,
	MID_VarMonsterX, MID_VarMonsterY, MID_VarMonsterO,
	MID_VarAccessory, MID_VarMonsterXP, MID_VarMonsterXPMult,
	MID_VarItemHPMult, MID_VarItemATKMult, MID_VarItemDEFMult, MID_VarItemGRMult,
	MID_TotalMoves, MID_TotalTime,
	MID_VarMonsterParamX, MID_VarMonsterParamY, MID_VarMonsterParamW, MID_VarMonsterParamH, MID_VarMonsterParamF,
	MID_VarHotTile, MID_VarExplosion,
	0, 0, 0, 0,
	MID_VarEnemyHP, MID_VarEnemyAtk, MID_VarEnemyDef, MID_VarEnemyGold, MID_VarEnemyXP,
	MID_VarWeaponATK, MID_VarWeaponDEF, MID_VarWeaponGR,
	MID_VarArmorATK, MID_VarArmorDEF, MID_VarArmorGR,
	MID_VarAccessoryATK, MID_VarAccessoryDEF, MID_VarAccessoryGR,
	MID_VarMyMonsterHPMult, MID_VarMyMonsterATKMult, MID_VarMyMonsterDEFMult, MID_VarMyMonsterGRMult, MID_VarMyMonsterXPMult,
	MID_VarMyItemMult, MID_VarMyItemHPMult, MID_VarMyItemATKMult, MID_VarMyItemDEFMult, MID_VarMyItemGRMult,
	MID_VarMudSpawn, MID_VarTarSpawn, MID_VarGelSpawn, MID_VarQueenSpawn
};

string ScriptVars::midTexts[PredefinedVarCount]; //inited on first call

//*****************************************************************************
//Global game vars.  A subset of the predefined vars.
const Predefined ScriptVars::globals[numGlobals] = {
	P_HP,
	P_ATK,
	P_DEF,
	P_GOLD,
	P_XP,
	P_YKEY,
	P_GKEY,
	P_BKEY,
	P_SKEY,
	P_SWORD,
	P_SHIELD,
	P_ACCESSORY,
	P_ITEM_MULT,
	P_ITEM_HP_MULT,
	P_ITEM_ATK_MULT,
	P_ITEM_DEF_MULT,
	P_ITEM_GR_MULT,
	P_MONSTER_HP_MULT,
	P_MONSTER_ATK_MULT,
	P_MONSTER_DEF_MULT,
	P_MONSTER_GOLD_MULT,
	P_MONSTER_XP_MULT,
	P_HOTTILE,
	P_EXPLOSION,
	P_TOTALMOVES,
	P_TOTALTIME,
	P_MUD_SPAWN,
	P_TAR_SPAWN,
	P_GEL_SPAWN,
	P_QUEEN_SPAWN
};

//The MIDs for the global var subset.
const UINT ScriptVars::globalVarMIDs[numGlobals] = {
	predefinedVarMIDs[0],  //basic stats
	predefinedVarMIDs[1],
	predefinedVarMIDs[2],
	predefinedVarMIDs[3],
	predefinedVarMIDs[15],

	predefinedVarMIDs[4],  //keys
	predefinedVarMIDs[5],
	predefinedVarMIDs[6],
	predefinedVarMIDs[22],

	predefinedVarMIDs[7],  //equipment
	predefinedVarMIDs[8],
	predefinedVarMIDs[29],

	predefinedVarMIDs[20], //game multipliers
	predefinedVarMIDs[32],
	predefinedVarMIDs[33],
	predefinedVarMIDs[34],
	predefinedVarMIDs[35],

	predefinedVarMIDs[16], //monster stat multipliers
	predefinedVarMIDs[17],
	predefinedVarMIDs[18],
	predefinedVarMIDs[19],
	predefinedVarMIDs[31],

	predefinedVarMIDs[43], //damage modifiers
	predefinedVarMIDs[44],

	predefinedVarMIDs[36], //tally stats
	predefinedVarMIDs[37],

	predefinedVarMIDs[73], //monster spawn IDs
	predefinedVarMIDs[74],
	predefinedVarMIDs[75],
	predefinedVarMIDs[76]
};

//Match the global var texts in 'globalVarMIDs' to their short form texts in
//'predefinedVarTexts'.
const char* ScriptVars::globalVarShortNames[numGlobals] = {
	predefinedVarTexts[0],  //basic stats
	predefinedVarTexts[1],
	predefinedVarTexts[2],
	predefinedVarTexts[3],
	predefinedVarTexts[15],

	predefinedVarTexts[4],  //keys
	predefinedVarTexts[5],
	predefinedVarTexts[6],
	predefinedVarTexts[22],

	predefinedVarTexts[7],  //equipment
	predefinedVarTexts[8],
	predefinedVarTexts[29],

	predefinedVarTexts[20], //game multipliers
	predefinedVarTexts[32],
	predefinedVarTexts[33],
	predefinedVarTexts[34],
	predefinedVarTexts[35],

	predefinedVarTexts[16], //monster stat multipliers
	predefinedVarTexts[17],
	predefinedVarTexts[18],
	predefinedVarTexts[19],
	predefinedVarTexts[31],

	predefinedVarTexts[43], //damage modifiers
	predefinedVarTexts[44],

	predefinedVarTexts[36], //tally stats
	predefinedVarTexts[37],

	predefinedVarTexts[73], //monster spawn IDs
	predefinedVarTexts[74],
	predefinedVarTexts[75],
	predefinedVarTexts[76]

};

//*****************************************************************************
string ScriptVars::getVarName(const Predefined var)
//Returns: pointer to the name of this pre-defined var, or empty string if no match
{
	if (var < FirstPredefinedVar || var >= 0)
		return string();

	init(); //ensure texts are populated

	const UINT index = -var - 1;
	ASSERT(index < PredefinedVarCount);
	return midTexts[index];
}

//*****************************************************************************
WSTRING ScriptVars::getVarNameW(const Predefined var)
//Returns: pointer to the name of this pre-defined var, or NULL if no match
{
	const string varName = getVarName(var);

	WSTRING wstr;
	UTF8ToUnicode(varName, wstr);
	return wstr;
}

//*****************************************************************************
void ScriptVars::init()
//Init 'midTexts' on first call.
//Much faster than repeated multiple DB queries.
{
	UINT index=0;
	if (midTexts[0].empty())
	{
		for (int i=-1; i>=FirstPredefinedVar; --i, ++index)
		{
			//Get user-readable form of var.
			ASSERT(index < PredefinedVarCount);
			const WCHAR *pText = g_pTheDB->GetMessageText(predefinedVarMIDs[index]);
			midTexts[index] = UnicodeToUTF8(pText);
		}
		ASSERT(!midTexts[0].empty());
	}
}

//*****************************************************************************
Predefined ScriptVars::parsePredefinedVar(const WSTRING& wstr)
//Returns: the enumeration for this variable name, or P_NoVar if not recognized
{
	const string str = UnicodeToUTF8(wstr);
	return parsePredefinedVar(str);
}

//*****************************************************************************
Predefined ScriptVars::parsePredefinedVar(const string& str)
{
	init();

	const char *pText = str.c_str();
	UINT index=0;
	for (int i=-1; i>=FirstPredefinedVar; --i, ++index)
	{
/* //the user doesn't know about these string literals
		if (!stricmp(pText, predefinedVarTexts[index]))
			return Predefined(i);
*/

		//Compare against user-readable form of var.
		if (!_stricmp(pText, midTexts[index].c_str()))
			return Predefined(i);
	}
	return P_NoVar;
}

//*****************************************************************************
UINT PlayerStats::getVar(const Predefined var) const
//Gets specified var's value
{
	switch (var)
	{
		case P_HP: return this->HP;
		case P_ATK: return this->ATK;
		case P_DEF: return this->DEF;
		case P_GOLD: return this->GOLD;
		case P_XP: return this->XP;
		case P_YKEY: return this->yellowKeys;
		case P_GKEY: return this->greenKeys;
		case P_BKEY: return this->blueKeys;
		case P_SKEY: return this->skeletonKeys;
		case P_SWORD: return this->sword;
		case P_SHIELD: return this->shield;
		case P_ACCESSORY: return this->accessory;
		case P_SPEED: return this->speed;

		case P_MONSTER_HP_MULT: return this->monsterHPmult;
		case P_MONSTER_ATK_MULT: return this->monsterATKmult;
		case P_MONSTER_DEF_MULT: return this->monsterDEFmult;
		case P_MONSTER_GOLD_MULT: return this->monsterGRmult;
		case P_MONSTER_XP_MULT: return this->monsterXPmult;

		case P_ITEM_MULT: return this->itemMult;
		case P_ITEM_HP_MULT: return this->itemHPmult;
		case P_ITEM_ATK_MULT: return this->itemATKmult;
		case P_ITEM_DEF_MULT: return this->itemDEFmult;
		case P_ITEM_GR_MULT: return this->itemGRmult;

		case P_HOTTILE: return this->hotTileVal;
		case P_EXPLOSION: return this->explosionVal;

		case P_TOTALMOVES: return this->totalMoves;
		case P_TOTALTIME: return this->totalTime;

		case P_MUD_SPAWN: return this->mudSpawnID;
		case P_TAR_SPAWN: return this->tarSpawnID;
		case P_GEL_SPAWN: return this->gelSpawnID;
		case P_QUEEN_SPAWN: return this->queenSpawnID;

		case P_NoVar:
		default: return 0;
	}
}

//*****************************************************************************
void PlayerStats::setVar(const Predefined var, const UINT val)
//Sets specified var to given value
{
	switch (var)
	{
		case P_HP: this->HP = val; break;
		case P_ATK: this->ATK = int(val); break;
		case P_DEF: this->DEF = int(val); break;
		case P_GOLD: this->GOLD = int(val); break;
		case P_XP: this->XP = int(val); break;
		case P_YKEY: this->yellowKeys = val; break;
		case P_GKEY: this->greenKeys = val; break;
		case P_BKEY: this->blueKeys = val; break;
		case P_SKEY: this->skeletonKeys = val; break;
		case P_SWORD: this->sword = val; break;
		case P_SHIELD: this->shield = val; break;
		case P_ACCESSORY: this->accessory = val; break;
		case P_SPEED: this->speed = val; break;

		case P_MONSTER_HP_MULT: this->monsterHPmult = val; break;
		case P_MONSTER_ATK_MULT: this->monsterATKmult = val; break;
		case P_MONSTER_DEF_MULT: this->monsterDEFmult = val; break;
		case P_MONSTER_GOLD_MULT: this->monsterGRmult = val; break;
		case P_MONSTER_XP_MULT: this->monsterXPmult = val; break;

		case P_ITEM_MULT: this->itemMult = val; break;
		case P_ITEM_HP_MULT: this->itemHPmult = val; break;
		case P_ITEM_ATK_MULT: this->itemATKmult = val; break;
		case P_ITEM_DEF_MULT: this->itemDEFmult = val; break;
		case P_ITEM_GR_MULT: this->itemGRmult = val; break;

		case P_HOTTILE: this->hotTileVal = val; break;
		case P_EXPLOSION: this->explosionVal = val; break;

		case P_TOTALMOVES: this->totalMoves = val; break;
		case P_TOTALTIME: this->totalTime = val; break;

		case P_MUD_SPAWN: this->mudSpawnID = int(val); break;
		case P_TAR_SPAWN: this->tarSpawnID = int(val); break;
		case P_GEL_SPAWN: this->gelSpawnID = int(val); break;
		case P_QUEEN_SPAWN: this->queenSpawnID = int(val); break;

		case P_NoVar:
		default: break;
	}
}

//***************************************************************************************
//Returns: true if index corresponds to a case that is used in Pack/Unpack below
bool PlayerStats::IsGlobalStatIndex(UINT i)
{
	return
		i < 10 ||
		(i >= 15 && i <= 20) ||
		i == 22 ||
		i == 29 ||
		(i >= 31 && i <= 37) ||
		(i >= 43 && i <= 48) ||
		(i >= 73 && i <= 76)
		;
}

//***************************************************************************************
void PlayerStats::Pack(CDbPackedVars& stats)
//Writes player (and global) RPG stats to the stats buffer.
{
	for (UINT i=PredefinedVarCount; i--; )
	{
		if (!IsGlobalStatIndex(i))
			continue; //these values are not player/global stats

		UINT val;
		switch (i)
		{
			//These case values correspond to the ordering of values in the 'Predefined' enumeration,
			//and not the (negative) enumeration values themselves.
			case 0: val = this->HP; break;
			case 1: val = UINT(this->ATK); break;
			case 2: val = UINT(this->DEF); break;
			case 3: val = UINT(this->GOLD); break;
			case 15: val = UINT(this->XP); break;

			case 4: val = this->yellowKeys; break;
			case 5: val = this->greenKeys; break;
			case 6: val = this->blueKeys; break;
			case 22: val = this->skeletonKeys; break;

			case 7: val = this->sword; break;
			case 8: val = this->shield; break;
			case 29: val = this->accessory; break;
			case 9: val = this->speed; break;

			case 16: val = this->monsterHPmult; break;
			case 17: val = this->monsterATKmult; break;
			case 18: val = this->monsterDEFmult; break;
			case 19: val = this->monsterGRmult; break;
			case 31: val = this->monsterXPmult; break;

			case 20: val = this->itemMult; break;
			case 32: val = this->itemHPmult; break;
			case 33: val = this->itemATKmult; break;
			case 34: val = this->itemDEFmult; break;
			case 35: val = this->itemGRmult; break;

			case 43: val = this->hotTileVal; break;
			case 44: val = this->explosionVal; break;

			case 36: val = this->totalMoves; break;
			case 37: val = this->totalTime; break;

			case 45: val = this->priorRoomID; break;
			case 46: val = this->priorX; break;
			case 47: val = this->priorY; break;
			case 48: val = this->priorO; break;

			case 73: val = UINT(this->mudSpawnID); break;
			case 74: val = UINT(this->tarSpawnID); break;
			case 75: val = UINT(this->gelSpawnID); break;
			case 76: val = UINT(this->queenSpawnID); break;

			default:
				ASSERT(!"Not a global var index");
				val = UINT(0); //should not be written
			break;
		}

		ASSERT(predefinedVarTexts[i][0] != 0); //not empty string
		stats.SetVar(predefinedVarTexts[i], val);
	}
}

//***************************************************************************************
void PlayerStats::Unpack(CDbPackedVars& stats)
//Reads player (and global) RPG stats from the stats buffer.
{
	for (UINT i=PredefinedVarCount; i--; )
	{
		if (!IsGlobalStatIndex(i))
			continue; //these values are not player/global stats

		ASSERT(predefinedVarTexts[i][0] != 0); //not empty string
		UINT defaultVal = 0;
		if (73 <= i && i <= 76)
			defaultVal = UINT(-1);
		const UINT val = stats.GetVar(predefinedVarTexts[i], defaultVal);
		switch (i)
		{
			//These case values correspond to the ordering of values in the 'Predefined' enumeration,
			//and not the (negative) enumeration values themselves.
			case 0: this->HP = val; break;
			case 1: this->ATK = int(val); break;
			case 2: this->DEF = int(val); break;
			case 3: this->GOLD = int(val); break;
			case 15: this->XP = int(val); break;

			case 4: this->yellowKeys = val; break;
			case 5: this->greenKeys = val; break;
			case 6: this->blueKeys = val; break;
			case 22: this->skeletonKeys = val; break;

			case 7: this->sword = val; break;
			case 8: this->shield = val; break;
			case 29: this->accessory = val; break;
			case 9: this->speed = val; break;

			case 16: this->monsterHPmult = val; break;
			case 17: this->monsterATKmult = val; break;
			case 18: this->monsterDEFmult = val; break;
			case 19: this->monsterGRmult = val; break;
			case 31: this->monsterXPmult = val; break;

			case 20: this->itemMult = val; break;
			case 32: this->itemHPmult = val; break;
			case 33: this->itemATKmult = val; break;
			case 34: this->itemDEFmult = val; break;
			case 35: this->itemGRmult = val; break;

			case 43: this->hotTileVal = val; break;
			case 44: this->explosionVal = val; break;

			case 36: this->totalMoves = val; break;
			case 37: this->totalTime = val; break;

			case 45: this->priorRoomID = val; break;
			case 46: this->priorX = val; break;
			case 47: this->priorY = val; break;
			case 48: this->priorO = val; break;

			case 73: this->mudSpawnID = int(val); break;
			case 74: this->tarSpawnID = int(val); break;
			case 75: this->gelSpawnID = int(val); break;
			case 76: this->queenSpawnID = int(val); break;

			default: ASSERT(!"Bad var"); break;
		}
	}
}
