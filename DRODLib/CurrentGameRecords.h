#ifndef CURRENTGAMERECORDS_H
#define CURRENTGAMERECORDS_H

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>
#include <map>
#include <string>

//*******************************************************************************
struct VarMapInfo {
	bool bInteger;
	UINT val;
	WSTRING wstrVal;
};
typedef std::string VarNameType;
typedef std::map<VarNameType, VarMapInfo> VARMAP;

struct LevelExit {
	enum Type {
		StairLookup,
		SpecificID
	};

	LevelExit() : type(StairLookup), entranceID(0) { }
	LevelExit(Type type, UINT entranceID) : type(type), entranceID(entranceID) { }

	Type type;
	UINT entranceID;

	static bool IsWorldMapID(UINT entranceID) { return int(entranceID) < 0; }
	static UINT ConvertWorldMapID(UINT entranceID) { return static_cast<UINT>(-int(entranceID)); }
};

struct WorldMapMusic
{
	WorldMapMusic(UINT songID, UINT customID=0)
		: songID(songID), customID(customID)
	{ }
	WorldMapMusic(const WSTRING& songlist)
		: songID(0), customID(0), songlist(songlist)
	{ }

	UINT songID;
	UINT customID;
	WSTRING songlist;
};

#endif
