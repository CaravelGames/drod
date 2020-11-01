#include "PlayerStats.h"
#include "Db.h"
#include "CueEvents.h"
#include "../Texts/MIDs.h"

using namespace ScriptVars;

//*****************************************************************************
//Symbols of predefined in-game variables.
const UINT ScriptVars::predefinedVarMIDs[PredefinedVarCount] = {
	MID_VarMonsterWeapon,
	0, 0,
	MID_VarX, MID_VarY, MID_VarO,
	MID_VarMonsterX, MID_VarMonsterY, MID_VarMonsterO,
	0, 0,
	MID_VarMonsterParamX, MID_VarMonsterParamY, MID_VarMonsterParamW, MID_VarMonsterParamH, MID_VarMonsterParamF,
	MID_VarRoomImageX, MID_VarRoomImageY, MID_VarOverheadImageX, MID_VarOverheadImageY,
	MID_VarLevelName, MID_VarThreatClock, MID_VarPlayerLight, MID_VarPlayerLightType,
	MID_VarReturnX, MID_VarReturnY
};

string ScriptVars::midTexts[PredefinedVarCount]; //inited on first call

//*****************************************************************************
string ScriptVars::getVarName(const Predefined var)
//Returns: pointer to the name of this pre-defined var, or empty string if no match
{
	if (var < FirstPredefinedVar)
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
	string varName = getVarName(var);

	WSTRING wstr;
	AsciiToUnicode(varName.c_str(), wstr);
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
			UnicodeToUTF8(pText, midTexts[index]);
		}
		ASSERT(!midTexts[0].empty());
	}
}

//*****************************************************************************
bool ScriptVars::IsStringVar(Predefined val)
{
	return val == P_LEVELNAME;
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

	//Optimization: all predefined vars start with an underscore
	if (pText[0] == '_') {
		UINT index=0;
		for (int i=-1; i>=FirstPredefinedVar; --i, ++index)
		{
			//Compare against user-readable form of var.
			if (!_stricmp(pText, midTexts[index].c_str()))
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
	return wstr && wstr[0] == '.';
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
