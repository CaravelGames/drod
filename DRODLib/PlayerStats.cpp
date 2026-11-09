#include "PlayerStats.h"
#include "Db.h"
#include "CueEvents.h"
#include "../Texts/MIDs.h"

using namespace ScriptVars;

//*****************************************************************************
//Symbols of predefined in-game variables. - Must be in the same order as Predefined enum
const string ScriptVars::varNames[PredefinedVarCount] = {
	"_MyWeapon",
	"", //unused
	"_MyColor",
	"_X",
	"_Y",
	"_O",
	"_MyX",
	"_MyY",
	"_MyO",
	"", //unused
	"", //unused
	"_MyScriptX",
	"_MyScriptY",
	"_MyScriptW",
	"_MyScriptH",
	"_MyScriptF",
	"_RoomImageX",
	"_RoomImageY",
	"_OverheadImageX",
	"_OverheadImageY",
	"_LevelName",
	"_ThreatClock",
	"_PlayerLight",
	"_PlayerLightType",
	"_ReturnX",
	"_ReturnY",
	"_MyName",
	"_RoomX",
	"_RoomY",
	"_RoomWeather",
	"_RoomDarkness",
	"_RoomFog",
	"_RoomSnow",
	"_RoomRain",
	"_SpawnCycle",
	"_SpawnCycleFast",
	"_PlayerWeapon",
	"_PlayerLocalWeapon",
	"_Input",
	"_InputDirection",
	"_Combo",
	"_MyHue",
	"_MySaturation",
};

//*****************************************************************************
// Values are not case sensitive; caps added here for readability
const char ScriptVars::primitiveNames[PrimitiveCount][15] =
{
	"_abs",
	"_min",
	"_max",
	"_orient",
	"_facing",
	"_ox",
	"_oy",
	"_rotateCW",
	"_rotateCCW",
	"_rotateDist",
	"_dist0",
	"_dist1",
	"_dist2",
	"_ArrowDir",
	"_RoomTile",
	"_MonsterType",
	"_CharacterType",
	"_EntityWeapon",
	"_MonsterSize",
	"_BrainScore",
	"_BrainDist",
	"_CleanRooms"
};

//*****************************************************************************
string ScriptVars::getVarName(const Predefined var)
//Returns: pointer to the name of this pre-defined var, or empty string if no match
{
	if (var < FirstPredefinedVar)
		return string();

	const UINT index = -var - 1;
	ASSERT(index < PredefinedVarCount);
	return varNames[index];
}

//*****************************************************************************
WSTRING ScriptVars::getVarNameW(const Predefined var)
//Returns: pointer to the name of this pre-defined var, or NULL if no match
{
	string varName = getVarName(var);

	WSTRING wstr;
	UTF8ToUnicode(varName.c_str(), wstr);
	return wstr;
}

//*****************************************************************************
PrimitiveType ScriptVars::parsePrimitive(const WSTRING& wstr)
{
	const string str = UnicodeToUTF8(wstr);
	return parsePrimitive(str);
}

PrimitiveType ScriptVars::parsePrimitive(const string& str)
{
	for (int i = 0; i < PrimitiveCount; ++i) {
		if (!_stricmp(str.c_str(), primitiveNames[i]))
			return PrimitiveType(i);
	}
	return NoPrimitive;
}

//Returns: the number of parameter arguments each primitive function requires
UINT ScriptVars::getPrimitiveRequiredParameters(PrimitiveType eType)
{
	switch (eType)
	{
		case P_Abs:
		case P_OrientX:
		case P_OrientY:
		case P_RotateCW:
		case P_RotateCCW:
		case P_CleanRooms:
			return 1;
		case P_Min:
		case P_Max:
		case P_Orient:
		case P_Facing:
		case P_RotateDist:
		case P_ArrowDir:
		case P_MonsterType:
		case P_CharacterType:
		case P_EntityWeapon:
		case P_MonsterSize:
			return 2;
		case P_RoomTile:
		case P_BrainScore:
			return 3;
		case P_Dist0:
		case P_Dist1:
		case P_Dist2:
			return 4;
		case P_BrainDist:
			return 5;
		case NoPrimitive:
		case PrimitiveCount:
			return 0;
	}
	return 0;
}

//*****************************************************************************
bool ScriptVars::IsStringVar(Predefined val)
{
	return val == P_LEVELNAME || val == P_MONSTER_NAME;
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
	const char *pText = str.c_str();

	//Optimization: all predefined vars start with an underscore
	if (pText[0] == '_') {
		UINT index=0;
		for (int i=-1; i>=FirstPredefinedVar; --i, ++index)
		{
			//Compare against user-readable form of var.
			if (!_stricmp(pText, varNames[index].c_str()))
				return Predefined(i);
		}
	}
	return P_NoVar;
}

//*****************************************************************************
bool ScriptVars::IsCharacterLocalVar(const WSTRING& wstr)
{
	return IsCharacterLocalVar(wstr.c_str());
}

bool ScriptVars::IsCharacterLocalVar(const WCHAR* wstr)
{
	return wstr && (wstr[0] == '.' || wstr[0] == '@');
}

bool ScriptVars::IsCharacterArrayVar(const WSTRING& wstr)
{
	return IsCharacterArrayVar(wstr.c_str());
}

bool ScriptVars::IsCharacterArrayVar(const WCHAR* wstr)
{
	return wstr && (wstr[0] == '#' || wstr[0] == '@');
}

//*****************************************************************************
bool ScriptVars::IsIndexInArrayRange(const int index)
{
	return abs(index) <= 50000;
}

//*****************************************************************************
void Challenges::deserialize(CDbPackedVars& vars)
{
	challenges.clear();

	for (UNPACKEDVAR *pVar = vars.GetFirst(); pVar != NULL; pVar = vars.GetNext())
	{
		//Each var is of the form "<holdID>-<unique ID>" --> <name>
		const char *name = pVar->name.c_str();
		if (!isdigit(name[0])) //robustness filter
			continue;

		const UINT holdID = static_cast<UINT>(atoi(name));
		if (!holdID)
			continue;

		const WSTRING wstr = vars.GetVar(name, wszEmpty);
		challenges[holdID].insert(wstr);
	}
}

void Challenges::serialize(CDbPackedVars& vars) const
{
	char temp[16];
	vars.Clear();
	for (ChallengeT::const_iterator it=challenges.begin(); it!=challenges.end(); ++it)
	{
		const UINT holdID = it->first;
		string varPrefix = _itoa(holdID, temp, 10);
		varPrefix.append("-");

		const HoldChallenges& holdChallenges = it->second;
		UINT uniqueID=0;
		for (HoldChallenges::const_iterator holdIt=holdChallenges.begin();
				holdIt!=holdChallenges.end(); ++holdIt, ++uniqueID)
		{
			const string varName = varPrefix + _itoa(uniqueID, temp, 10);
			const WSTRING& challengeName = *holdIt;
			vars.SetVar(varName.c_str(), challengeName.c_str());
		}
	}
}

set<WSTRING> Challenges::get(const UINT holdID) const
{
	ChallengeT::const_iterator it=challenges.find(holdID);
	return it != challenges.end() ? it->second : set<WSTRING>();
}

set<UINT> Challenges::getHoldIDs() const
{
	set<UINT> ids;
	for (ChallengeT::const_iterator it=challenges.begin(); it!=challenges.end(); ++it)
		ids.insert(it->first);
	return ids;
}

void Challenges::rekeyHoldIDs(const map<UINT,UINT>& holdIDmap)
{
	ChallengeT newChallenges;
	for (ChallengeT::const_iterator it=challenges.begin(); it!=challenges.end(); ++it)
	{
		const UINT holdID = it->first;
		map<UINT,UINT>::const_iterator mapIt = holdIDmap.find(holdID);
		if (mapIt == holdIDmap.end())
			continue;
		const UINT newHoldID = mapIt->second;
		newChallenges[newHoldID] = it->second;
	}

	this->challenges = newChallenges;
}

bool Challenges::add(const UINT holdID, CCueEvents* CueEvents)
{
	ASSERT(CueEvents);
	set<WSTRING> challengesCompleted;
	Challenges::GetFrom(CueEvents, challengesCompleted);

	return add(holdID, challengesCompleted);
}

//Returns: true if new names were added to the set for the indicated holdID
bool Challenges::add(const UINT holdID, const set<WSTRING>& names)
{
	if (!names.empty()) {
		const size_t num = challenges[holdID].size();
		challenges[holdID].insert(names.begin(), names.end());
		return challenges[holdID].size() > num;
	}
	return false;
}

Challenges& Challenges::operator+=(const Challenges& rhs)
{
	for (ChallengeT::const_iterator it = rhs.challenges.begin();
			it != rhs.challenges.end(); ++it)
	{
		const HoldChallenges& holdChallenges = it->second;
		challenges[it->first].insert(holdChallenges.begin(), holdChallenges.end());
	}
	return *this;
}

void Challenges::GetFrom(CCueEvents* CueEvents, set<WSTRING>& challengesCompleted)
{
	for (const CAttachableObject *pObj = CueEvents->GetFirstPrivateData(CID_ChallengeCompleted);
			pObj != NULL; pObj = CueEvents->GetNextPrivateData())
	{
		const CAttachableWrapper<WSTRING> *pText =
				dynamic_cast<const CAttachableWrapper<WSTRING>*>(pObj);	//challenge name
		ASSERT(pText);
		challengesCompleted.insert(pText->data);
	}
}
