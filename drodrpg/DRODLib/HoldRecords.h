#ifndef HOLDRECORDS_H
#define HOLDRECORDS_H

#include "CharacterCommand.h"
#include "DbPackedVars.h"
#include "GameConstants.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/IDSet.h>

#include <map>
#include <vector>

//Literals used to query and store values for Hold Characters in the packed vars object.
#define scriptIDstr "ScriptID"

//*****************************************************************************
//To facilitate DB queries.
struct HoldStats
{
	CIDSet rooms, secretRooms; //requiredRooms
};

//*****************************************************************************
//Per-hold script vars.
struct HoldVar
{
	HoldVar()
		: dwVarID(0)
	{ }
	HoldVar(const UINT dwVarID, const WCHAR* pwszName)
		: dwVarID(dwVarID)
	{
		ASSERT(pwszName);
		this->varNameText = pwszName;
	}
	void clear()
	{
		dwVarID = 0;
		varNameText.clear();
	}

	UINT dwVarID;  //unique ID
	WSTRING varNameText;
};

//*****************************************************************************
//Per-hold world maps.
struct HoldWorldMap
{
	enum DisplayType {
		NoLabels = 0,
		Labels = 1,
		LabelsWhenExplored = 2
	};

	HoldWorldMap()
		: worldMapID(0), dataID(0), displayType(Labels), orderIndex(0)
	{
	}
	HoldWorldMap(const UINT worldMapID, const UINT dataID,
		DisplayType displayType, UINT orderIndex, const WCHAR* pwszName)
		: worldMapID(worldMapID), dataID(dataID), displayType(displayType)
		, orderIndex(orderIndex)
	{
		ASSERT(pwszName);
		this->nameText = pwszName;
	}
	void clear()
	{
		worldMapID = dataID = orderIndex = 0;
		displayType = NoLabels;
		nameText.clear();
	}

	UINT worldMapID;  //unique ID
	UINT dataID; //foreign key
	DisplayType displayType;
	UINT orderIndex;
	WSTRING nameText;
};

struct sortWorldMaps {
	bool operator() (const HoldWorldMap* pMap1, const HoldWorldMap* pMap2) const
	{
		return pMap1->orderIndex < pMap2->orderIndex;
	}
};
typedef std::set<const HoldWorldMap*, sortWorldMaps> SORTED_WORLD_MAPS;

//*****************************************************************************
//Per-hold definable NPC character.
struct HoldCharacter
{
	HoldCharacter()
		: dwCharID(0), wType(0), dwDataID_Avatar(0), dwDataID_Tiles(0)
		, animationSpeed(0)
		, pCommands(NULL)
	{ }
	HoldCharacter(const HoldCharacter& that, const bool bReplicateData=false)
		: dwCharID(that.dwCharID), charNameText(that.charNameText), wType(that.wType)
		, dwDataID_Avatar(that.dwDataID_Avatar), dwDataID_Tiles(that.dwDataID_Tiles)
		, animationSpeed(that.animationSpeed), ExtraVars(that.ExtraVars)
		, pCommands(NULL)
	{
		if (that.pCommands)
		{
			this->pCommands = new COMMANDPTR_VECTOR;
			const UINT numCommands = that.pCommands->size();
			for (UINT i=0; i<numCommands; ++i)
			{
				CCharacterCommand *pOldCommand = (*that.pCommands)[i];
				CCharacterCommand *pCommand = new CCharacterCommand(*pOldCommand, bReplicateData);
				this->pCommands->push_back(pCommand);
			}
		}
	}
	HoldCharacter(const UINT dwCharID, const WCHAR* pwszName,
			const UINT wType=0, const UINT dwDataID_Avatar=0,
			const UINT dwDataID_Tiles=0, const UINT animationSpeed=0,
			const CDbPackedVars& ExtraVars=CDbPackedVars())
		: dwCharID(dwCharID), wType(wType)
		, dwDataID_Avatar(dwDataID_Avatar), dwDataID_Tiles(dwDataID_Tiles)
		, animationSpeed(animationSpeed), ExtraVars(ExtraVars)
		, pCommands(NULL)
	{
		ASSERT(pwszName);
		this->charNameText = pwszName;
	}
	~HoldCharacter()
	{
		deleteWorkingCommands();
	}

	void clear()
	{
		dwCharID = dwDataID_Avatar = dwDataID_Tiles = 0;
		wType = animationSpeed = 0;
		charNameText.clear();
		ExtraVars.Clear();
	}

	void deleteWorkingCommands()
	{
		if (this->pCommands)
		{
			for (COMMANDPTR_VECTOR::iterator command = this->pCommands->begin();
					command != this->pCommands->end(); ++command)
				delete *command;
			delete this->pCommands;
			this->pCommands = NULL;
		}
	}

	UINT dwCharID;  //unique ID
	WSTRING charNameText;
	UINT wType;  //character's analogous monster type
	UINT dwDataID_Avatar; //avatar image ref (optional)
	UINT dwDataID_Tiles;  //tile set image ref (optional)
	UINT animationSpeed;  //how fast to animate custom tile set, if defined
	CDbPackedVars ExtraVars; //holds default character script
	COMMANDPTR_VECTOR *pCommands; //working copy of default character script
};

//******************************************************************************************
//Where an exit can go
enum ExitType
{
	ET_Entrance = 0,
	ET_WorldMap = 1,
};

//*****************************************************************************
struct WorldMapIcon
{
	WorldMapIcon()
		: entranceID(0), xPos(0), yPos(0), charID(0), imageID(0), displayFlags(0)
		, exitType(ExitType::ET_Entrance)
	{
	}
	WorldMapIcon(UINT entranceID, UINT xPos, UINT yPos, UINT charID, UINT imageID, UINT displayFlags,
		ExitType exitType)
		: entranceID(entranceID), xPos(xPos), yPos(yPos), charID(charID), imageID(imageID), displayFlags(displayFlags)
		, exitType(exitType)
	{
	}

	void clear() { entranceID = xPos = yPos = charID = imageID = displayFlags = 0; exitType = ExitType::ET_Entrance; }
	bool IsTraverserable() const {
		switch (displayFlags) {
			case ScriptFlag::WMI_On:
			case ScriptFlag::WMI_LevelState:
			case ScriptFlag::WMI_Cleared: return true;
			default: return false;
		}
	}
	bool IsWorldMapExit() const { return exitType == ExitType::ET_WorldMap; }

	UINT entranceID;
	ExitType exitType;
	UINT xPos, yPos;
	UINT charID, imageID;
	UINT displayFlags;
};
typedef std::vector<WorldMapIcon> WorldMapIcons; //icons on a world map
typedef std::map<UINT, WorldMapIcons> WorldMapsIcons; //worldMapID --> icons

#endif
