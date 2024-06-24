#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include "DbPackedVars.h"
#include <BackEndLib/Wchar.h>
#include <map>
#include <set>
#include <string>
using std::map;
using std::set;
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
		LessThanOrEqual=8, //to avoid collision with Op values
		GreaterThanOrEqual=9,
		Inequal=10
	};

	//Predefined global and relative game state vars, accessed through these key values.
	enum Predefined
	{
		P_NoVar  =  0,
		P_MONSTER_WEAPON = -1,
		//P_SWORD = -2,
		P_MONSTER_COLOR = -3,
		P_PLAYER_X = -4,
		P_PLAYER_Y = -5,
		P_PLAYER_O = -6,
		P_MONSTER_X = -7,
		P_MONSTER_Y = -8,
		P_MONSTER_O = -9,
		//P_TOTALMOVES = -10,
		//P_TOTALTIME = -11,
		P_SCRIPT_X = -12,
		P_SCRIPT_Y = -13,
		P_SCRIPT_W = -14,
		P_SCRIPT_H = -15,
		P_SCRIPT_F = -16,
		P_ROOMIMAGE_X = -17,
		P_ROOMIMAGE_Y = -18,
		P_OVERHEADIMAGE_X = -19,
		P_OVERHEADIMAGE_Y = -20,
		P_LEVELNAME = -21,
		P_THREATCLOCK = -22,
		P_PLAYERLIGHT = -23,
		P_PLAYERLIGHTTYPE = -24,
		P_RETURN_X = -25,
		P_RETURN_Y = -26,
		P_MONSTER_NAME = -27,
		P_ROOM_X = -28,
		P_ROOM_Y = -29,
		P_ROOM_WEATHER = -30,
		P_ROOM_DARKNESS = -31,
		P_ROOM_FOG = -32,
		P_ROOM_SNOW = -33,
		P_ROOM_RAIN = -34,
		P_SPAWNCYCLE = -35,
		P_SPAWNCYCLE_FAST = -36,
		P_PLAYER_WEAPON = -37,
		P_PLAYER_LOCAL_WEAPON = -38,
		P_INPUT = -39,
		P_INPUT_DIRECTION = -40,
		FirstPredefinedVar = P_INPUT_DIRECTION, //set this to the last var in the enumeration
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
		P_Facing,    //(dx,dy) --> o
		P_OrientX,   //o --> dx
		P_OrientY,   //o --> dy
		P_RotateCW,  //o --> cw(o)
		P_RotateCCW, //o --> ccw(o)
		P_RotateDist,//(o1, o2) --> number of turns between the two
		P_Dist0,     //L-infinity norm
		P_Dist1,     //L-1 norm (Manhattan distance)
		P_Dist2,     //L-2 norm (Euclidean distance)
		P_ArrowDir,  //(x,y) --> direction of arrow at (x,y)
		P_RoomTile,  //(x,y,z) --> id of room tile at (x,y) on layer z
		P_MonsterType,//(x,y) --> type of monster at (x,y), or -1 if no monster is there
		P_CharacterType,//(x,y) --> appearance of character at (x,y) or -1 if no character is there
		P_EntityWeapon,//(x,y) --> weapon type of monster/player at (x,y), or -1 if no weapon holder is there
		P_BrainScore,//(x,y,t) --> pathmap score of tile at (x,y) for movement type t
		PrimitiveCount
	};

	void init();
	string getVarName(const ScriptVars::Predefined var);
	WSTRING getVarNameW(const ScriptVars::Predefined var);
	bool IsStringVar(Predefined val);
	Predefined parsePredefinedVar(const string& str);
	Predefined parsePredefinedVar(const WSTRING& wstr);

	PrimitiveType parsePrimitive(const string& str);
	UINT getPrimitiveRequiredParameters(PrimitiveType eType);
	PrimitiveType parsePrimitive(const WSTRING& wstr);

	bool IsCharacterLocalVar(const WSTRING& wstr);
	bool IsCharacterLocalVar(const WCHAR* wstr);
	bool IsCharacterArrayVar(const WSTRING& wstr);
	bool IsCharacterArrayVar(const WCHAR* wstr);

	//All predefined vars.
	extern const UINT predefinedVarMIDs[PredefinedVarCount];
	extern string midTexts[PredefinedVarCount];
	extern const char primitiveNames[PrimitiveCount][15]; //expand buffer size as needed
};

//Stats used for various tally operations.
class RoomStats
{
public:
	RoomStats() {clear();}
	virtual ~RoomStats() {}

	virtual void clear() {
		yellowDoors = greenDoors = blueDoors = redDoors = blackDoors = 0;
		openYellowDoors = openGreenDoors = openBlueDoors = openRedDoors = openBlackDoors = 0;
		rooms = secrets = levels = 0;
	}

	UINT yellowDoors, greenDoors, blueDoors, redDoors, blackDoors;
	UINT openYellowDoors, openGreenDoors, openBlueDoors, openRedDoors, openBlackDoors;
	UINT rooms, secrets, levels;
};

class CCueEvents;
struct Challenges
{
	Challenges() { }
	Challenges(CDbPackedVars& vars) { deserialize(vars); }

	void clear() { challenges.clear(); }
	void deserialize(CDbPackedVars& vars);
	void serialize(CDbPackedVars& vars) const;

	set<UINT> getHoldIDs() const;
	set<WSTRING> get(const UINT holdID) const;
	void rekeyHoldIDs(const map<UINT,UINT>& holdIDmap);

	bool add(const UINT holdID, CCueEvents* CueEvents);
	bool add(const UINT holdID, const set<WSTRING>& names);
	Challenges& operator+=(const Challenges& rhs);

	static void GetFrom(CCueEvents* CueEvents, set<WSTRING>& challengesCompleted);

private:
	typedef set<WSTRING> HoldChallenges;
	typedef map<UINT, HoldChallenges> ChallengeT;
	ChallengeT challenges; 
};

#endif
