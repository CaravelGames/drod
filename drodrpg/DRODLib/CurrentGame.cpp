// $Id: CurrentGame.cpp 10216 2012-05-20 08:36:59Z skell $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//CurrentGame.cpp.
//Implementation of CCurrentGame.

#include "CurrentGame.h"
#include "Ant.h"
#include "Combat.h"
#include "Db.h"
#include "DbProps.h"
#include "DbXML.h"
#include "CueEvents.h"
#include "GameConstants.h"
#include "Halph.h"
#include "Monster.h"
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "Character.h"
#include "Clone.h"
#include "EvilEye.h"
#include "Mimic.h"
#include "Pathmap.h"
#include "PhoenixAshes.h"
#include "Serpent.h"
#include "Splitter.h"
#include "Swordsman.h"
#include "TileConstants.h"
#include "NetInterface.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/SysTimer.h>
#include <BackEndLib/Wchar.h>

#include <limits>

//queue<DEMO_UPLOAD*> CCurrentGame::demosForUpload;
queue<SCORE_UPLOAD*> CCurrentGame::scoresForUpload;

//Game character/monster constant that speaker refers to.
const UINT SpeakerConstant[Speaker_Count] = {
	M_BEETHRO, /*M_HALPH*/ M_NEATHER, M_SLAYER, M_NEGOTIATOR, UINT(M_CUSTOM), UINT(M_CUSTOM),
	M_CITIZEN1, M_CITIZEN2, M_GOBLINKING, M_GOBLIN,
	M_INSTRUCTOR, M_MUDCOORDINATOR, M_ROCKGOLEM, M_TARTECHNICIAN, M_GUARD, M_MADEYE,
	M_CITIZEN, M_PIRATE, M_ROACH, M_QROACH, M_REGG, M_WWING, M_EYE,
	M_SERPENT, M_TARMOTHER, M_TARBABY, M_BRAIN, M_MIMIC, M_SPIDER,
	M_SERPENTG, M_SERPENTB, M_WATERSKIPPER, M_SKIPPERNEST, M_AUMTLICH, M_CLONE,
	M_DECOY, M_WUBBA, M_SEEP, M_FEGUNDO, M_FEGUNDOASHES, M_MUDMOTHER,
	M_MUDBABY, M_GELMOTHER, M_GELBABY, M_ROCKGIANT, M_CITIZEN3, M_CITIZEN4,
	M_BEETHRO_IN_DISGUISE, UINT(M_CUSTOM), UINT(M_PLAYER), M_STALWART
};

#define shieldOffStr    "ShieldOff"
#define accessoryOffStr "AccessoryOff"
#define commandOffStr   "CommandOff"

#define musicEnumStr   "MusicEnum"
#define musicIDStr     "MusicID"
#define playMusicIDStr "MusicPlayID"
#define songMoodStr    "MusicMood"

//*****************************************************************************
int intBounds(const float fVal)
//Performs bounds checking on a floating point value in order to return
//an integer value robust to potential overflow
//
//Return: an integer
{
	if (fVal <= INT_MIN)
		return INT_MIN; //lower bounds check
	if (fVal >= INT_MAX)
		return INT_MAX; //upper bounds check

	return int(fVal);
}

//*****************************************************************************
UINT UINTBounds(const float fVal)
//Performs bounds checking on a floating point value in order to return
//a UINT value robust to potential overflow
//
//Return: a UINT
{
	if (fVal <= 0)
		return 0; //lower bounds check
	if (fVal >= UINT_MAX)
		return UINT_MAX; //upper bounds check

	return UINT(fVal);
}

//*****************************************************************************
void incintValueWithBounds(int& val,
								const int inc) //may be either positive or negative
//Return: a sum of two values, robust to the possibility of overflow
{
	const float fSum = (float)val + (float)inc;
	if (fSum >= INT_MAX)
		val = INT_MAX; //max bounds checking
	else if (fSum <= INT_MIN)
		val = INT_MIN; //min bounds checking
	else
		val += inc;
}

//*****************************************************************************
void incUINTValueWithBounds(UINT& val,
								const int inc) //may be either positive or negative
//Return: a sum of two values, robust to the possibility of overflow
{
	const float fSum = (float)val + (float)inc;
	if (fSum >= INT_MAX)
		val = INT_MAX; //max bounds checking
	else if (fSum <= 0)
		val = 0; //min bounds checking
	else
		val += inc;
}

//
//Protected methods.
//

//*****************************************************************************
CCurrentGame::CCurrentGame()
//Constructor.
	: pRoom(NULL)
	, pLevel(NULL)
	, pHold(NULL)
	, pCombat(NULL)
//	, pSnapshotGame(NULL)
	, pPendingPriorLocation(NULL)
	, bNoSaves(false) // Clear() does not set this
	, bValidatingPlayback(false)
{
	//Zero resource members before calling Clear().
	this->pPlayer = new CSwordsman(this);

	Clear();
}

//
//Public methods.
//

//*****************************************************************************
CCurrentGame::~CCurrentGame()
//Destructor.
{
/*
	if (this->bIsDemoRecording)
	{
		if (!EndDemoRecording())
		{
			CFiles f;
			f.AppendErrorLog("Failed to save a demo when recording ended." NEWLINE);
		}
	}
*/
	
	if (this->Commands.IsFrozen())
		this->Commands.Unfreeze();

	Clear();
	delete this->pPlayer;
}

//*****************************************************************************
WSTRING CCurrentGame::AbbrevRoomLocation()
//Prepend room position to non-empty (player viewable) demo descriptions.
{
	WSTRING descText;

	//Hold name.
	WSTRING holdName = static_cast<const WCHAR *>(this->pHold->NameText);
	WSTRING abbrevHoldName;
	static const UINT MAX_HOLD_NAME = 8;
	if (holdName.size() <= MAX_HOLD_NAME)
		descText += holdName;
	else
	{
		//Try to abbreviate by taking only the first letter from each word
		abbrevHoldName = filterFirstLettersAndNumbers(holdName);
		descText += abbrevHoldName;
	}
	descText += wszColon;
	descText += wszSpace;

	//Level name.
	descText += this->pLevel->NameText;
	descText += wszColon;
	descText += wszSpace;

	//Room name.
	WSTRING abbrevRoomPosition;
	this->pRoom->GetLevelPositionDescription(abbrevRoomPosition, true);
	descText += abbrevRoomPosition;

	return descText;
}

//*****************************************************************************
void CCurrentGame::activateCustomEquipment(
//Creates a script instance for this custom equipment and inserts it into the
//global script list.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT type,         //type of inventory
	const UINT newEquipment) //equipment ID
{
	ASSERT(bIsCustomEquipment(newEquipment));
	if (!this->pHold->GetCharacter(newEquipment))
		return; //nothing to activate if this is an invalid custom character type

	CMonsterFactory mf;
	CMonster *pNew = mf.GetNewMonster((MONSTERTYPE)M_CHARACTER);
	ASSERT(pNew);
	pNew->wX = pNew->wY = static_cast<UINT>(-1); //not located anywhere in the room area
	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
	pCharacter->wLogicalIdentity = newEquipment;
	pCharacter->equipType = (ScriptFlag::EquipmentType)type;
	pCharacter->SetCurrentGame(this); //will assign the default script for this custom NPC
	pCharacter->dwScriptID = getNewScriptID();
	appendToGlobalMonsterList(pCharacter);

	//Set up the equipment's properties by running the script.
	pCharacter->Process(CMD_WAIT, CueEvents);
}

//*****************************************************************************
void CCurrentGame::ActivateTokenAt(const UINT wX, const UINT wY)
//Marks the token at (x,y) to be activated at the right time during the
//current move's logic flow.
{
	this->pendingTokenActivations.push_back(CCoord(wX, wY));
}

//*****************************************************************************
void CCurrentGame::AddNewEntity(
//Adds a new monster in the room of the indicated type.
//
//Params:
	CCueEvents& CueEvents,
	const UINT identity, const UINT wX, const UINT wY, const UINT wO)
{
	if (!IsValidOrientation(wO))
		return; //invalid
	if (bIsSerpent(identity) || identity == M_FEGUNDOASHES || identity == M_CHARACTER)
		return; //not supported
	if (IsValidMonsterType(identity))
	{
		CMonster *pMonster = this->pRoom->AddNewMonster(identity, wX, wY);
		const bool bHasOrientation = pMonster->HasOrientation();
		pMonster->wO = bHasOrientation ? wO : NO_ORIENTATION;
		if (bHasOrientation && pMonster->wO == NO_ORIENTATION)
			pMonster->wO = NW; //default
		pMonster->bIsFirstTurn = true;

		//Affect tile being placed on.
		if (this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE && !bIsEntityFlying(identity))
			this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);
		return;
	}

	if (identity == M_NONE)
	{
		//Remove any entity occupying this tile.
		this->pRoom->KillMonsterAtSquare(wX, wY, CueEvents, true);
		return;
	}

	if (identity >= CUSTOM_CHARACTER_FIRST &&
			!this->pHold->GetCharacter(identity))
		return; //do nothing if this is an invalid custom character type

	//Add NPC to the room.
	CMonsterFactory mf;
	CMonster *pNew = mf.GetNewMonster((MONSTERTYPE)M_CHARACTER);
	ASSERT(pNew);
	pNew->wX = pNew->wPrevX = wX;
	pNew->wY = pNew->wPrevY = wY;
	pNew->wO = pNew->wPrevO = wO;
	pNew->bIsFirstTurn = true; //don't reprocess until next turn

	//Set up NPC info.
	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
	pCharacter->wLogicalIdentity = identity;
	pCharacter->SetCurrentGame(this); //will assign the default script for custom NPCs
	pCharacter->dwScriptID = getNewScriptID();

	//Place in room if visible.
	bool bVisible = pCharacter->IsVisible();
	if (bVisible &&
			(this->pRoom->GetMonsterAtSquare(wX, wY) || IsPlayerAt(wX, wY)))
	{
		//When the room tile is already occupied, the NPC must begin invisible.
		if (pCharacter->commands.empty())
		{
			//There is no default script for the NPC, so it would never appear.
			//Just pretend it was never added to the room.
			delete pCharacter;
			return;
		}

		pCharacter->bVisible = false;
		bVisible = false;
	}
	this->pRoom->LinkMonster(pNew, bVisible);

	//Affect tile being placed on.
	if (this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE && !pCharacter->IsFlying())
		this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);

	//Set up the NPC's properties by running the default script.
	//Guard against creating new scripted entities that immediately
	//create new scripted entities, ad infinitum.
	static const UINT MAX_ADD_NEW_ENTITY_RECURSE_DEPTH = 3;
	if (this->wAddNewEntityDepth >= MAX_ADD_NEW_ENTITY_RECURSE_DEPTH)
	{
		//Log the occurrence and don't let the new NPC run any script commands
		//so that the recursion won't continue.
		string str = "AddNewEntity: Recursive depth limit reached.  ID = ";
		char temp[16];
		str += _itoa(identity, temp, 10);
		str += NEWLINE;
		CFiles f;
		f.AppendUserLog(str.c_str());
	} else {
		const bool bExec = ExecutingNoMoveCommands();
		SetExecuteNoMoveCommands();

		++this->wAddNewEntityDepth;
		pCharacter->Process(CMD_WAIT, CueEvents);
		--this->wAddNewEntityDepth;

		SetExecuteNoMoveCommands(bExec);
	}
}

//*****************************************************************************
void CCurrentGame::AddRoomToMap(
//Adds this roomID to the player's map if it's not already explored.
//
//Params:
	const UINT roomID,
	const bool bMarkRoomVisible, //If set [default=false], then make the room fully visible on the map.
	const bool bSaveRoom) //Whether to flag including this room in save data [default=true]
{
	ExploredRoom *pExpRoom = getExploredRoom(roomID);
	if (pExpRoom)
	{
		pExpRoom->bSave |= bSaveRoom; //saving takes precedence
	} else {
		pExpRoom = new ExploredRoom();
		this->ExploredRooms.push_back(pExpRoom);
		pExpRoom->roomID = roomID;
		pExpRoom->bMapOnly = true;
		pExpRoom->bSave = bSaveRoom;
	}

	//Room marked only on the map becomes fully visible (as if explored) if bMarkRoomVisible is set.
	ASSERT(pExpRoom);
	if (bMarkRoomVisible && pExpRoom->bMapOnly)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(roomID);
		if (pRoom)
		{
			//Temporarily point the current game to the new room
			//so it can be set up properly.
			CDbRoom *pOrigRoom = this->pRoom;
			this->pRoom = pRoom;

			pRoom->SetCurrentGame(this); //init room+monster stats

/*		NO -- not allowed, as running scripts in remote rooms could cause all kinds of bad side-effects.
			//Init NPCs and sworded monsters after initial room state checks and modifications are performed.
			CCueEvents Ignored; //don't display anything firing in rooms being revealed
			const bool bExecOriginal = this->bExecuteNoMoveCommands;
			this->bExecuteNoMoveCommands = true;
			pRoom->PreprocessMonsters(Ignored);
			this->bExecuteNoMoveCommands = bExecOriginal;
*/

			SaveExploredRoomData(*pRoom);

			this->pRoom = pOrigRoom;
			delete pRoom;
		}
	}
}

//*****************************************************************************
bool CCurrentGame::Autosave(const WSTRING& name)
//Creates an "autosave" save record with the specified name and adds it to the DB.
//
//Returns: whether a record was saved to the DB
{
	if (this->bNoSaves)
		return false;

	if (this->Commands.IsFrozen() || !this->bIsGameActive)
		return false; //don't save anything when commands are frozen or game isn't active

	SaveGame(ST_Autosave, name);
	return true;
}

//*****************************************************************************
bool CCurrentGame::changingInventory(
//Call when a new piece of inventory is being acquired.
//
//Returns: whether inventory trade was executed
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT type,         //type of inventory
	const UINT newEquipment) //equipment ID
{
	if (this->changingInventoryTypes.has(type))
		return false;

	//Remove any old custom script running for this equipment type.
	removeGlobalScriptForEquipment(type);

	if (bIsCustomEquipment(newEquipment))
	{
		this->changingInventoryTypes += type;
		activateCustomEquipment(CueEvents, type, newEquipment);
		this->changingInventoryTypes -= type;
	}
	return true;
}

//*****************************************************************************
void CCurrentGame::getEquipmentStats(
//Gets the ATK + DEF stats for the equipment type wielded by the player.
//
//Params:
	const UINT type,
	int& ATKstat, int &DEFstat) //(out) ATK, DEF values of this equipment item
const
{
	ATKstat=DEFstat=0;
	PlayerStats& st = this->pPlayer->st;
	const CCharacter *pCharacter = getCustomEquipment(type);
	switch (type)
	{
		case ScriptFlag::Weapon:
			ATKstat = getWeaponPower(st.sword);
			if (pCharacter)
				DEFstat = (int)pCharacter->getDEF();
		break;
		case ScriptFlag::Armor:
			DEFstat = getShieldPower(st.shield);
			if (pCharacter)
				ATKstat = (int)pCharacter->getATK();
		break;
		case ScriptFlag::Accessory:
			if (pCharacter)
			{
				ATKstat = (int)pCharacter->getATK();
				DEFstat = (int)pCharacter->getDEF();
			}
		break;
		default: break;
	}
}

//*****************************************************************************
float CCurrentGame::GetGlobalStatModifier(ScriptVars::StatModifiers statType) const
// Returns: the multiplicative global modifier for the given statistic
{
	PlayerStats& st = this->pPlayer->st;

	// When calculating, treat values that can be negative as signed int
	switch (statType) {
		case ScriptVars::StatModifiers::MonsterHP:
			return st.monsterHPmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterATK:
			return st.monsterATKmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterDEF:
			return st.monsterDEFmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterGR:
			return int(st.monsterGRmult) / 100.0f;
		case ScriptVars::StatModifiers::MonsterXP:
			return int(st.monsterXPmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemAll:
			return int(st.itemMult) / 100.0f;
		case ScriptVars::StatModifiers::ItemHP:
			return int(st.itemHPmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemATK:
			return int(st.itemATKmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemDEF:
			return int(st.itemDEFmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemGR:
			return int(st.itemGRmult) / 100.0f;
		default:
			return 1.0f;
	}

	return 1.0f;
}

//*****************************************************************************
float CCurrentGame::GetTotalStatModifier(ScriptVars::StatModifiers statType) const
//Returns: the total multiplicative modifier for the given statistic
//It is the product of the global, NPC and equipment modifiers
{
	float fMult = GetGlobalStatModifier(statType);
	fMult *= pRoom->GetStatModifierFromCharacters(statType);

	const CCharacter* pWeapon = getCustomEquipment(ScriptFlag::Weapon);
	if (pWeapon) {
		fMult *= pWeapon->GetStatModifier(statType);
	}
	const CCharacter* pArmor = getCustomEquipment(ScriptFlag::Armor);
	if (pArmor) {
		fMult *= pArmor->GetStatModifier(statType);
	}
	const CCharacter* pAccessory = getCustomEquipment(ScriptFlag::Accessory);
	if (pAccessory) {
		fMult *= pAccessory->GetStatModifier(statType);
	}

	return fMult;
}

//*****************************************************************************
bool CCurrentGame::IsEquipmentValid(
	const UINT id, // (in) Index/ID of the equipment
	const UINT type) // (in) type of the equipment (ScriptFlag::EquipmentType constant)
{
	switch ((ScriptFlag::EquipmentType)type){
		case (ScriptFlag::Weapon):
			return bIsValidStandardWeapon(id) || this->pHold->GetCharacter(id);
		break;
		case (ScriptFlag::Armor):
			return bIsValidStandardShield(id) || this->pHold->GetCharacter(id);
		break;
		case (ScriptFlag::Accessory):
			return bIsValidStandardAccessory(id) || this->pHold->GetCharacter(id);
		break;

		default: return false;
	}
}

//*****************************************************************************
UINT CCurrentGame::getNewScriptID()
//Returns: a unique scriptID to give an NPC script for the game session
{
	if (this->CompletedScripts.empty())
	{
		//Start with a unique script #.
		const UINT max = this->pHold->GetScriptID() + 1;
		this->CompletedScripts += max;
		return max;
	}

	//Increment the max script #.
	ASSERT(this->CompletedScripts.size() == 1); //this set is being used to store an incremented ID for now
	UINT max = this->CompletedScripts.getMax();
	this->CompletedScripts -= max;
	++max;
	this->CompletedScripts += max;
	return max;
}

//*****************************************************************************
void CCurrentGame::SaveGame(const SAVETYPE eSaveType, const WSTRING& name)
//Save game of specified type with the current info.
{
	ASSERT(!this->bNoSaves);

	const bool bScoreSubmission = eSaveType == ST_ScoreCheckpoint;

	//If we're submitting a score, save the game record with the current stats.
	//Otherwise, we must store the state that game was in on room entrance,
	//so when moves for the current room are replayed, the current state is recreated.
	CDbPackedVars _stats = this->stats;
	if (!bScoreSubmission)
		this->stats = this->statsAtRoomStart;

	if (eSaveType == ST_Autosave)
	{
		//Set saved game ID to current player's autosave slot for this hold and specified name.
		//Overwrite any previous matching autosave.
		this->dwSavedGameID = g_pTheDB->SavedGames.FindByName(eSaveType, name);
	}
	else
		this->dwSavedGameID = 0; //always save a new record
	this->wVersionNo = VERSION_NUMBER;
	this->eType = eSaveType;
	this->stats.SetVar(szSavename, name.c_str());
	this->bIsHidden = bScoreSubmission;
	Update();

	this->stats = _stats; //revert
}

//*****************************************************************************
/*
void CCurrentGame::BeginDemoRecording(
//Begins demo recording.  Database writes to store the demo will occur when the 
//swordsman exits a room or EndDemoRecording() is called.
//
//Params:
	const WCHAR *pwczSetDescription, //(in)  Description stored with demo record(s).
	const bool bUseCurrentTurnNo)    //[default=true]
{
	ASSERT(pwczSetDescription);
	ASSERT(!this->bIsDemoRecording);
	
	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
	ASSERT(!this->Commands.IsFrozen());

	//Set recording information.
	this->DemoRecInfo.dwDescriptionMessageID = AddMessageText(pwczSetDescription);
	this->DemoRecInfo.wBeginTurnNo = bUseCurrentTurnNo ? this->wTurnNo : 0;
	this->DemoRecInfo.dwPrevDemoID = 0L;
	this->DemoRecInfo.dwFirstDemoID = 0L;
	this->DemoRecInfo.SetFlag(CDbDemo::Usermade);

	this->bIsDemoRecording = true;
}
*/

//*****************************************************************************
void CCurrentGame::Clear(
//Frees resources associated with this object and zeros members.
//
//Params:
	const bool bNewGame)  //(in)   whether new game is starting [default=true]
{
	CDbSavedGame::Clear(bNewGame);  //Resets explored rooms.

	delete this->pRoom;
	this->pRoom = NULL;

	delete this->pLevel;
	this->pLevel = NULL;

	if (bNewGame)
	{
		//Only reset the hold and explored rooms if a new game is beginning
		delete this->pHold;
		this->pHold=NULL;
//		this->bHoldMastered = false;
	}

	this->pEntrance = NULL;

	this->pPlayer->Clear(bNewGame);

	this->wSpawnCycleCount = this->wPlayerTurn = this->wTurnNo = 0;
	this->dwStartTime = 0;
	this->bHalfTurn = false;

	this->pendingTokenActivations.clear();
	this->simulSwordHits.clear();
	this->possibleTarStabs.clear();
	this->changingInventoryTypes.clear();
	delete this->pPendingPriorLocation;
	this->pPendingPriorLocation = NULL;

	this->ambientSounds.clear();
	for (vector<CCharacterCommand*>::const_iterator iter = this->roomSpeech.begin();
			iter != this->roomSpeech.end(); ++iter)
		delete *iter;
	this->roomSpeech.clear();
	this->music.clear();

/*
	this->wMonsterKills = 0L;
	this->wMonstersKilledRecently = 0L;
	this->bLotsOfMonstersKilled = false;
*/
	this->bExecuteNoMoveCommands = false;
	this->wAddNewEntityDepth = 0;

/*
	this->bBrainSensesSwordsman = false;
	this->wLastCheckpointX = this->wLastCheckpointY = static_cast<UINT>(-1);
	this->checkpointTurns.clear();

	this->dwAutoSaveOptions = ASO_DEFAULT;
	this->bIsDemoRecording = false;
*/
	this->bIsNewRoom = false;
	
	this->bIsGameActive = false;

	this->dwCutScene = 0;
//	this->bWaitedOnHotFloorLastTurn = false;

//	memset(&(this->DemoRecInfo), 0, sizeof(this->DemoRecInfo));

	this->UnansweredQuestions.clear();

/*
	this->bRoomExitLocked = false;

	delete this->pSnapshotGame;
	this->pSnapshotGame = NULL;
	this->dwComputationTime = 0;
	this->dwComputationTimePerSnapshot = 500; //ms
*/

	delete this->pCombat;
	this->pCombat = NULL;
	this->pBlockedSwordHit = NULL;
	this->bQuickCombat = false;
}

//*****************************************************************************
void CCurrentGame::DiffVarValues(const VARMAP& vars1, const VARMAP& vars2, set<VarNameType>& diff)
//Outputs the set of vars that are different between vars1 and vars2.
//
//ASSUME: vars2 is a superset of vars1
{
	diff.clear();

	for (VARMAP::const_iterator var2 = vars2.begin(); var2 != vars2.end(); ++var2)
	{
		VARMAP::const_iterator var1 = vars1.find(var2->first);
		if (var1 != vars1.end())
		{
			//Var is present in both maps.  Check type and value.
			const VarMapInfo& v1 = var1->second;
			const VarMapInfo& v2 = var2->second;
			if (v1.bInteger == v2.bInteger)
			{
				//Same type.  Check value.
				if (v1.bInteger)
				{
					if (v1.val == v2.val)
						continue; //same
				} else {
					if (!v1.wstrVal.compare(v2.wstrVal))
						continue; //same
				}
			}
		} else {
			//Var is absent from vars1, but if value in vars2 is set to the default, then ignore it.
			const VarMapInfo& v2 = var2->second;
			if (v2.bInteger)
			{
				if (!v2.val)
					continue; //default value -- ignore
			} else {
				if (v2.wstrVal.empty())
					continue; //default value -- ignore
			}
		}

		//This var is different.
		diff.insert(var2->first);
	}
}

//*****************************************************************************
/*
UINT CCurrentGame::EndDemoRecording()
//Ends demo recording, which may cause database to be updated with demo information.
//
//Returns:
//DemoID of first and maybe only demo in series of demos recorded (one per room) or 
//TAG_ESCAPE if no commands recorded.
{
#define TAG_ESCAPE   (UINT)(-2)    //same as in FrontEndLib/Widget.*

	ASSERT(this->bIsDemoRecording);

	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
	ASSERT(!this->Commands.IsFrozen());

	this->bIsDemoRecording = false;

	//If no commands were recorded, no database update for the current room is needed.
	if (this->DemoRecInfo.wBeginTurnNo >= this->wTurnNo)
		return TAG_ESCAPE;

	//Save the current room state and commands to a demo.
	WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

	return this->DemoRecInfo.dwFirstDemoID;
#undef TAG_ESCAPE
}
*/

//*****************************************************************************
void CCurrentGame::ExitCurrentRoom()
//Invoke this method whenever a room is exited during play.
{
	if (!this->Commands.IsFrozen() && !this->bNoSaves)
	{
		//Add movement commands executed within this room to the play session's
		//movement sequence.
		//Only do this if we're not in the middle of a game session begun before version 1.1,
		//when this feature was implemented.
		//This is indicated by having already explored rooms while having an empty moves object.
		if (this->roomsExploredAtRoomStart.empty() || //first room in the game session
				!this->moves.empty()) //play sequence is being stored
			this->moves.Append(this->Commands);
	}

	//Record that scripts were completed.
	AddCompletedScripts();

	//Save info for room being exited.
	SaveExploredRoomData(*this->pRoom);
}

//*****************************************************************************
WSTRING CCurrentGame::ExpandText(
//Translate escape sequences embedded in text string to literal values.
//
//Params:
	const WCHAR* wText,     //text to expand
	CCharacter *pCharacter) //character, to query its local vars [default=NULL]
{
	if (!wText)
		return WSTRING();

	WSTRING wStr, wEscapeStr;
	UINT wIndex;
	bool bEscapeSeq = false;
	for (wIndex=0; wText[wIndex] != 0; ++wText)
	{
		if (wText[wIndex] == '$')
		{
			if ((bEscapeSeq = !bEscapeSeq))
				continue; //just started what looks like an escape sequence
			if (wEscapeStr.empty())
				wStr += wszDollarSign; //$$
			else
			{
				//Process newline escape sequence.
				static const WCHAR newlineEscape[] = { We('\\'),We('n'),We(0) };
				if (!WCScmp(newlineEscape, wEscapeStr.c_str()))
				{
					wStr += wszCRLF;
				} else {
					//Resolve var name.
					WCHAR wIntText[20];
					const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wEscapeStr);
					InputCommands::DCMD reserved_lookup_id;
					if (eVar != ScriptVars::P_NoVar)
					{
						//Is it a predefined var?
						UINT val;
						if (pCharacter)
							val = pCharacter->getPredefinedVar(eVar);
						else
							val = getVar(eVar);
						wStr += _itoW(int(val), wIntText, 10);
					} else if ((reserved_lookup_id = InputCommands::getCommandIDByVarName(wEscapeStr)) < InputCommands::DCMD_Count) {
						//Is it a player input button?
						wStr += getTextForInputCommandKey(reserved_lookup_id);
					} else {
						//Is it a hold-local var?
						const UINT varID = this->pHold->GetVarID(wEscapeStr.c_str());
						if (varID)
						{
							//Yes -- get its value, if defined.
							char *varName = this->pHold->getVarAccessToken(wEscapeStr.c_str());
							const UNPACKEDVARTYPE vType = this->stats.GetVarType(varName);
							const bool bExistingIntValue = vType == UVT_int || vType == UVT_uint;
							if (bExistingIntValue)
							{
								//Integer.
								const int nVal = this->stats.GetVar(varName, int(0));
								wStr += _itoW(nVal, wIntText, 10);
							} else if (vType == UVT_wchar_string) {
								//A text string.
								wStr += this->stats.GetVar(varName, wszEmpty);
							}
						} else {
							//Might be a complex expression.  Does it parse?
							UINT index=0;
							if (CCharacter::IsValidExpression(wEscapeStr.c_str(), index, this->pHold))
							{
								index=0;
								const int nVal = CCharacter::parseExpression(wEscapeStr.c_str(), index, this);
								wStr += _itoW(nVal, wIntText, 10);
							}
						}
					}
				}
				wEscapeStr.resize(0);
			}
		} else {
			if (bEscapeSeq)
				wEscapeStr += wText[wIndex];
			else
				wStr += wText[wIndex];
		}
	}

	//If there are non-paired var name delimiters, just add this text to the string.
	if (!wEscapeStr.empty())
		wStr += wEscapeStr;

	return wStr;
}

//*****************************************************************************
WSTRING CCurrentGame::getTextForInputCommandKey(InputCommands::DCMD id) const
{
	ASSERT(id < InputCommands::DCMD_Count);

	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	const InputCommands::DCMD eCommand = InputCommands::DCMD(
			settings.GetVar(InputCommands::COMMANDNAME_ARRAY[id], 0));

	return g_pTheDB->GetMessageText(MID_UNKNOWN + eCommand);
}

//*****************************************************************************
void CCurrentGame::FreezeCommands()
//Disallow modification of command list, i.e. adding commands, clearing, or truncating.  
//Assertions will fire in CDbCommands if this is violated.
{
	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
//	ASSERT(!this->bIsDemoRecording);

	this->Commands.Freeze();
}

//*****************************************************************************
CCharacter* CCurrentGame::GetCharacterWithScriptID(const UINT scriptID)
//Returns: character monster with indicated (unique) scriptID, or NULL if not found
{
	CCharacter *pCharacter = this->pRoom->GetCharacterWithScriptID(scriptID);
	if (pCharacter)
		return pCharacter;

	for (CMonster *pMonster = this->pMonsterList ; pMonster != NULL;
			pMonster = pMonster->pNext)
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->dwScriptID == scriptID)
				return pCharacter; //found it
		}

	return NULL; //not found
}

//*****************************************************************************
UINT CCurrentGame::GetChecksum()
//Gets a checksum representing the current game state.  This checksum is meant to
//stay the same even if DROD's implementation changes radically, so only things that
//I'm confident will be consistently measurable in future versions are included.
//
//Returns:
//The checksum.
const
{
	const UINT dwSum =

		//Swordsman position.
		BYTE(this->pPlayer->wX) * 0x00000001 + //16 possible positions
		BYTE(this->pPlayer->wY) * 0x00000010 + //16 possible positions
		BYTE(this->pPlayer->wO) * 0x00000100 + //9+ possible positions

		//Number of monsters.
//		BYTE(this->pRoom->wMonsterCount) * 0x00000800 + //<~1024

		//Turn count.
		BYTE(this->wTurnNo) * 0x00000A00 +

/*
		//Conquered rooms.
		BYTE(this->ConqueredRooms.size()) * 0x01000000 +
*/

		//Explored rooms.
		BYTE(this->ExploredRooms.size()) * 0x00400000;

	return dwSum;
}

//*****************************************************************************
UINT CCurrentGame::GetRoomExitDirection(
//Returns: direction swordsman should exit the current room,
//based on his move and position.
//
//Params:
	const UINT wMoveO)      //(in)   Direction swordsman is exiting.
const
{
	switch (wMoveO)
	{
		case N: return N;

		case S: return S;

		case W: return W;

		case E: return E;

		case NW:
		return this->pPlayer->wY == 0 ? N : W;

		case NE:
		return this->pPlayer->wY == 0 ? N : E;

		case SW:
		return this->pPlayer->wY == this->pRoom->wRoomRows - 1 ? S : W;

		case SE:
		return this->pPlayer->wY == this->pRoom->wRoomRows - 1 ? S : E;

		default:
			return NO_ORIENTATION;  //bad orientation
	}
}

//*****************************************************************************
UINT CCurrentGame::GetScore() const
//Returns: player's current score
{
	PlayerStats st = this->pPlayer->st; //temp copy

	//Insert the current ATK/DEF levels to calculate score.
	st.ATK = getPlayerATK();
	st.DEF = getPlayerDEF();

	return CDbSavedGames::GetScore(st);
}

//*****************************************************************************
WSTRING CCurrentGame::GetScrollTextAt(const UINT wX, const UINT wY)
//Returns: pointer to interpolated text of scroll at indicated room tile
{
	const WCHAR *pText = pRoom->GetScrollTextAtSquare(wX, wY);
	return ExpandText(pText);
}

//*****************************************************************************
CEntity* CCurrentGame::getSpeakingEntity(CFiredCharacterCommand *pFiredCommand)
//Returns: pointer to the best entity to speak the specified scripted speech command.
//This may be a dead monster, so this should be checked for by the caller.
{
	ASSERT(pFiredCommand);
	ASSERT(pFiredCommand->pCommand);
	CDbSpeech *pSpeech = pFiredCommand->pCommand->pSpeech;
	ASSERT(pSpeech);
	if (!pSpeech)
		return NULL; //robustness

	//Determine monster speaker identity.
	SPEAKER speaker = (SPEAKER)pSpeech->wCharacter;
	//a custom speaker should already have been resolved before calling this
	if (speaker == Speaker_Custom)
		speaker = Speaker_None;

	//Determine whether a custom speaker is set.
	HoldCharacter *pCustomChar = NULL;
	if ((UINT)speaker >= (UINT)CUSTOM_CHARACTER_FIRST && (UINT)speaker != (UINT)M_NONE)
	{
		pCustomChar = this->pHold->GetCharacter(speaker);
		if (!pCustomChar) //custom ID not defined (dangling ID?) -- default to no speaker
			speaker = Speaker_None;
	}

	CMonster *pMonster = pFiredCommand->pSpeakingEntity; //assume the attached entity is speaking
	CEntity *pEntity = pMonster;
	if (speaker != Speaker_Self && speaker != Speaker_None)
	{
		//The attached entity may not be speaking.
		//Find the best fit entity that fits the speaker description.
		bool bFoundSpeaker = false;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsVisible())
			{
				if ((UINT(speaker) >= UINT(CUSTOM_CHARACTER_FIRST) && UINT(speaker) != UINT(M_NONE) &&
						(UINT(speaker) == pCharacter->wLogicalIdentity || UINT(speaker) == pCharacter->wIdentity)) || //custom characters match (either ID or in appearance)
						(speaker == getSpeakerType(MONSTERTYPE(pCharacter->GetIdentity())))) //stock speaker types match
				bFoundSpeaker = true;   //So we conclude the attached monster is speaking
			}
		}

		if (!bFoundSpeaker)
		{
			//The attached monster is not the one speaking.  Find the speaking monster.
			pEntity = this->pRoom->GetSpeaker(speaker >= Speaker_Count ?
					UINT(speaker) : SpeakerConstant[speaker]); //custom vs. stock character speaker
			//If none of the indicated monster type are present...
			if (!pEntity)
			{
				//...and a custom speaker is indicated,
				//look for an entity that matches its base type
				if (pCustomChar)
					pEntity = this->pRoom->GetSpeaker(pCustomChar->wType, true);
			}
			//Otherwise, default to having the attached monster speak.
			if (!pEntity)
				pEntity = pMonster;
		}
	}

	ASSERT(pEntity);
	return pEntity;
}

//*****************************************************************************
UINT CCurrentGame::GetSwordMovement() const
{
	return this->pPlayer->wSwordMovement; //note: set in ProcessPlayer
}

//*****************************************************************************
bool CCurrentGame::GetSwordsman(UINT& wSX, UINT& wSY, const bool /*bIncludeNonTarget*/) const
//OUT: position of Beethro (or equivalent targeted player) in the room, or (-1000,-1000) if absent
//
//Returns: whether Beethro or player target is present
{
//	if (bIncludeNonTarget || this->pPlayer->IsTarget())
	if (this->pPlayer->IsInRoom())
	{
		//Player is a monster target.
		wSX = this->pPlayer->wX;
		wSY = this->pPlayer->wY;
		return true;
	}

/*
	//Check whether an NPC is posing as the player.
	CMonster *pNPCBeethro = this->pRoom->GetNPCBeethro();
	if (pNPCBeethro)
	{
		wSX = pNPCBeethro->wX;
		wSY = pNPCBeethro->wY;
		return true;
	}
*/

	//Player not present.
	wSX = wSY = static_cast<UINT>(-1000);
	return false;
}

//*****************************************************************************
UINT CCurrentGame::getVar(const UINT varIndex) const
//Gets the value of a global var.
{
	ASSERT(this->pPlayer);
	const CSwordsman& player = *(this->pPlayer);

	switch (varIndex)
	{
		//Combat enemy stats.
		case (UINT)ScriptVars::P_ENEMY_HP:
		case (UINT)ScriptVars::P_ENEMY_ATK:
		case (UINT)ScriptVars::P_ENEMY_DEF:
		case (UINT)ScriptVars::P_ENEMY_GOLD:
		case (UINT)ScriptVars::P_ENEMY_XP:
		{
			//Return zero value if there is no current combat enemy.
			if (!InCombat())
				return 0;
			ASSERT(this->pCombat);
			CMonster *pMonster = this->pCombat->pMonster;
			ASSERT(pMonster);
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_ENEMY_HP: return pMonster->HP;
				case (UINT)ScriptVars::P_ENEMY_ATK: return pMonster->ATK;
				case (UINT)ScriptVars::P_ENEMY_DEF: return pMonster->DEF;
				case (UINT)ScriptVars::P_ENEMY_GOLD: return pMonster->GOLD;
				case (UINT)ScriptVars::P_ENEMY_XP: return pMonster->XP;
			}
		}
		return 0; //removes compiler warning

		//Player equipment values.
		case (UINT)ScriptVars::P_WEAPON_ATK:
			return getWeaponPower(player.st.sword);
		case (UINT)ScriptVars::P_ARMOR_DEF:
			return getShieldPower(player.st.shield);
		//The rest are non-zero only for custom equipment.
		case (UINT)ScriptVars::P_WEAPON_DEF:
		case (UINT)ScriptVars::P_ARMOR_ATK:
		case (UINT)ScriptVars::P_ACCESSORY_ATK:
		case (UINT)ScriptVars::P_ACCESSORY_DEF:
		case (UINT)ScriptVars::P_WEAPON_GR:
		case (UINT)ScriptVars::P_ARMOR_GR:
		case (UINT)ScriptVars::P_ACCESSORY_GR:
		{
			UINT type;
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_WEAPON_DEF: case (UINT)ScriptVars::P_WEAPON_GR: type = ScriptFlag::Weapon; break;
				case (UINT)ScriptVars::P_ARMOR_ATK: case (UINT)ScriptVars::P_ARMOR_GR: type = ScriptFlag::Armor; break;
				default: type = ScriptFlag::Accessory; break;
			}
			CCharacter* pCharacter = getCustomEquipment(type);
			if (pCharacter)
			{
				switch (varIndex)
				{
					case (UINT)ScriptVars::P_ARMOR_ATK: case (UINT)ScriptVars::P_ACCESSORY_ATK:
						return pCharacter->getATK();
					case (UINT)ScriptVars::P_WEAPON_DEF: case (UINT)ScriptVars::P_ACCESSORY_DEF:
						return pCharacter->getDEF();
					default:
						return pCharacter->getGOLD();
				}
			}
		}
		return 0;

		default:
			return player.st.getVar(ScriptVars::Predefined(varIndex));
	}
}

//*****************************************************************************
void CCurrentGame::GetVarValues(VARMAP& vars)
//Outputs a mapping of var name to value info.
{
	vars.clear();

	for (UNPACKEDVAR *pVar = this->stats.GetFirst(); pVar != NULL; pVar = this->stats.GetNext())
	{
		//Skip non-vars (i.e. not of format "v<varID>".
		if (pVar->name[0] != 'v')
			continue;

		//Get var name.
		const UINT wVarID = atoi(pVar->name.c_str() + 1); //skip the "v"
		const string varName = UnicodeToAscii(this->pHold->GetVarName(wVarID));

		const bool bInteger = pVar->eType == UVT_int;
		VarMapInfo info;
		if (bInteger)
		{
			info.bInteger = true;
			info.val = this->stats.GetVar(pVar->name.c_str(), (int)0);
		} else {
			info.bInteger = false;
			info.wstrVal = this->stats.GetVar(pVar->name.c_str(), wszEmpty);
		}
		vars[varName] = info;
	}
}

//*****************************************************************************
void CCurrentGame::GotoLevelEntrance(
//Leaves the level and goes to the level with the indicated entrance.
//
//Params:
	CCueEvents& CueEvents, const UINT wEntrance, const bool bSkipEntranceDisplay)
{
	ASSERT(wEntrance != (UINT)EXIT_LOOKUP);

	ProcessPlayer_HandleLeaveLevel(CueEvents, wEntrance, bSkipEntranceDisplay);
}

//*****************************************************************************
void CCurrentGame::InitRPGStats(PlayerStats& s)
//Initializes player RPG stats to what they are at the beginning of a game.
{
	s.HP = 500;
	s.ATK = 10;
	s.DEF = 10;
	s.GOLD = 0;
	s.speed = 100;

	s.yellowKeys = s.greenKeys = s.blueKeys = s.skeletonKeys = 0;
	s.sword = NoSword;
	s.shield = NoShield;
	s.accessory = NoAccessory;

	s.monsterHPmult = s.monsterATKmult = s.monsterDEFmult =
		s.monsterGRmult = s.monsterXPmult = 100; //100%

	s.itemMult = s.itemHPmult = s.itemATKmult = s.itemDEFmult =
		s.itemGRmult = 100; //100%

	s.hotTileVal = 5;     //5% damage
	s.explosionVal = 100; //100% damage (kill)

	s.totalMoves = s.totalTime = 0;
}

//*****************************************************************************
bool CCurrentGame::IsCurrentRoomExplored(const bool bConsiderCurrentRoom) const
{
	return CDbSavedGame::IsRoomExplored(this->pRoom->dwRoomID, bConsiderCurrentRoom);
}

//*****************************************************************************
bool CCurrentGame::ShowLevelStart() const
//Show the current game show a level entrance.
{
	if (this->wTurnNo != 0)
		return false; //not at an entrance if room play is already in progress

	//At a level entrance position that should be shown?
	CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
			this->pPlayer->wX, this->pPlayer->wY);
	if (!pEntrance)
		return false;
	
	switch (pEntrance->eShowDescription)
	{
		default:
		case CEntranceData::DD_No: return false;
		case CEntranceData::DD_Always: return true;
		case CEntranceData::DD_Once: return !this->entrancesExplored.has(pEntrance->dwEntranceID);
	}
}

//*****************************************************************************
bool CCurrentGame::IsPlayerAt(const UINT wX, const UINT wY) const
//Returns: whether player is at (x,y)
{
	if (!this->pPlayer->IsInRoom())
		return false;

	return wX == this->pPlayer->wX && wY == this->pPlayer->wY;
}

//*****************************************************************************
bool CCurrentGame::IsPlayerDying() const
//Returns: whether the current state indicates the player is dying
{
	return !this->pPlayer->bAlive;
}

//*****************************************************************************
bool CCurrentGame::IsPlayerSwordAt(const UINT wX, const UINT wY) const
//Returns: whether player's sword is at (x,y)
{
	if (!this->pPlayer->HasSword())
		return false;

	if (!this->pPlayer->IsInRoom())
		return false;

	return wX == this->pPlayer->GetSwordX() && wY == this->pPlayer->GetSwordY();
}

//*****************************************************************************
bool CCurrentGame::IsRoomAtCoordsExplored(
//Determines if a room in the current level has been explored in the current game.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of room to check.
const
//
//Returns:
//True if it has, false if it hasn't.
{
	const UINT dwRoomID = this->pLevel->GetRoomIDAtCoords(dwRoomX, dwRoomY);
	return CDbSavedGame::IsRoomExplored(dwRoomID);
}

//*****************************************************************************
bool CCurrentGame::LoadFromHold(
//Loads current game from the starting level and room of a specified hold.
//
//Params:
	const UINT dwHoldID,   //(in) Identifies hold that game will begin in.
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first
							//    step into the room.
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromHold");
	bool bSuccess=true;

	LoadPrep();

	//Load the hold.
	this->pHold = new CDbHold();
	if (!this->pHold) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pHold->Load(dwHoldID);
	if (!bSuccess) goto Cleanup;

	//Load the first level of hold.
	this->pLevel = this->pHold->GetStartingLevel();
	if (!this->pLevel) {bSuccess=false; goto Cleanup;}
	this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);
	ASSERT(this->pEntrance);
	this->wStartRoomX = pEntrance->wX;
	this->wStartRoomY = pEntrance->wY;
	this->wStartRoomO = pEntrance->wO;
	this->wStartRoomAppearance = defaultPlayerType(); //start as default player type
	this->bStartRoomSwordOff = false;       //start with sword by default
	this->pLevel->dwStartingRoomID = pEntrance->dwRoomID;
	this->roomsExploredAtRoomStart.clear();
	this->roomsMappedAtRoomStart.clear();

	InitRPGStats(this->pPlayer->st);
	PackData(this->statsAtRoomStart);

	//Load the first room of level.
	this->pRoom = this->pLevel->GetStartingRoom();
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}

	//Set swordsman to beginning of room.
	SetPlayerToRoomStart();

	this->wVersionNo = VERSION_NUMBER;
	this->checksumStr = g_pTheNet->GetChecksum(this, 1);
/*
	//Save to level-begin and room-begin slots.
	//ATTN: Do this before SetMembersAfterRoomLoad changes anything.
	if ((this->dwAutoSaveOptions & ASO_LEVELBEGIN)==ASO_LEVELBEGIN)
		SaveToLevelBegin();
	if ((this->dwAutoSaveOptions & ASO_ROOMBEGIN)==ASO_ROOMBEGIN) 
		SaveToRoomBegin();
*/			
	//Get members ready.
	SetMembersAfterRoomLoad(CueEvents);
	ProcessCommand_EndOfTurnEventHandling(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*****************************************************************************
bool CCurrentGame::LoadFromLevelEntrance(
//Loads current game from an entrance of a specified level.
//Pre-Cond: Hold specified is the same as the one already in memory.
//
//Params:
	const UINT dwHoldID,
	const UINT dwEntranceID,
	CCueEvents &CueEvents)
{
	LOGCONTEXT("CCurrentGame::LoadFromLevelEntrance");
	ASSERT(this->pHold);
	ASSERT(dwHoldID == this->pHold->dwHoldID);

	bool bSuccess=true;

	//Save stats for the level just exited.
	if (this->pLevel)
		PackData(this->stats);

	{
		//Retain these game states.
		const UINT wIdentity_ = this->pPlayer->wIdentity;
		const bool bSwordOff_ = this->pPlayer->bSwordOff;
		CMoveCoordEx *p = this->pPendingPriorLocation;
		this->pPendingPriorLocation = NULL;

		LoadPrep(false);

		SetPlayerRole(wIdentity_);
		this->pPlayer->bSwordOff = bSwordOff_;
		this->pPendingPriorLocation = p;
	}

	if (dwEntranceID == (UINT)EXIT_PRIOR_LOCATION)
	{
		//Return to last saved warp location.
		PlayerStats& st = this->pPlayer->st;
		ASSERT(st.priorRoomID);
		this->pRoom = g_pTheDB->Rooms.GetByID(st.priorRoomID);
		if (!this->pRoom) {bSuccess=false; goto Cleanup;}
		this->pLevel = g_pTheDB->Levels.GetByID(this->pRoom->dwLevelID);
		if (!this->pLevel) {bSuccess=false; goto Cleanup;}

		this->pPlayer->wX = this->pPlayer->wPrevX = st.priorX;
		this->pPlayer->wY = this->pPlayer->wPrevY = st.priorY;
		this->pPlayer->SetOrientation(st.priorO);
	} else {
		//Go to predefined level entrance.
		this->pEntrance = this->pHold->GetEntrance(dwEntranceID);
		ASSERTP(pEntrance, "Dangling level entrance ID");   //Corrupted DB if NULL

		//Load the room of the level entrance.
		this->pRoom = new CDbRoom();
		if (!this->pRoom) {bSuccess=false; goto Cleanup;}
		bSuccess = this->pRoom->Load(pEntrance->dwRoomID);
		if (!this->pRoom) {bSuccess=false; goto Cleanup;}

		//Load the level.
		this->pLevel = new CDbLevel();
		if (!this->pLevel) {bSuccess=false; goto Cleanup;}
		bSuccess = this->pLevel->Load(this->pRoom->dwLevelID);
		if (!bSuccess) goto Cleanup;

		//Set room start vars to entrance.
		this->pPlayer->wX = this->pPlayer->wPrevX = pEntrance->wX;
		this->pPlayer->wY = this->pPlayer->wPrevY = pEntrance->wY;
		this->pPlayer->SetOrientation(pEntrance->wO);
	}

	UnpackData(this->stats);

	//After returning to a prior location,
	//set the next prior location warp to where the player came from.
	if (this->pPendingPriorLocation) {
		CMoveCoordEx *p = this->pPendingPriorLocation;
		PlayerStats& st = this->pPlayer->st;
		st.priorX = p->wX;
		st.priorY = p->wY;
		st.priorO = p->wO;
		st.priorRoomID = p->wValue;
		delete this->pPendingPriorLocation;
		this->pPendingPriorLocation = NULL;
	}

	SetRoomStartToPlayer();

	//Replace a previous LevelBegin saved game record if the level is being
	//entered for the first time in the current game, meaning if none of
	//the rooms in the level has ever been visited in the current game.
//	bool bNewLevel, bNewRoom;
//	CIDSet roomsInLevel = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
//	bNewLevel = !this->ExploredRooms.containsAny(roomsInLevel);
//	bNewRoom = !this->ExploredRooms.has(this->pRoom->dwRoomID);

	SetPlayerToRoomStart();

	//Get members ready.
	RetrieveExploredRoomData(*this->pRoom);
	CDbSavedGame::SetMonsterListAtRoomStart();
	SetMembersAfterRoomLoad(CueEvents);
	ProcessCommand_EndOfTurnEventHandling(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*****************************************************************************
bool CCurrentGame::LoadFromRoom(
//Loads current game from specified room (for playtesting).
//Hold and level are implied by the specified room.
//
//Params:
	const UINT dwRoomID,   //(in) Identifies room that game will begin in.
	CCueEvents &CueEvents,  //(out)  Cue events generated by swordsman's first
							//    step into the room.
	const UINT wX, const UINT wY, const UINT wO, //(in) Player starting position
	const UINT wIdentity,	//player starting logical identity
	const bool bSwordOff,   //whether player has sword or not
	const bool bNoSaves)    //whether DB saves should be prevented [default=false]
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromRoom");
	ASSERT(dwRoomID);

	bool bSuccess=true;

	this->bNoSaves = bNoSaves;
	LoadPrep();

	//Only checkpoint saves enabled during testing.
//	this->dwAutoSaveOptions = ASO_CHECKPOINT;

	//Load the room.
	this->pRoom = new CDbRoom;
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pRoom->Load(dwRoomID);
	if (!bSuccess) goto Cleanup;

	//Load level associated with room.
	this->pLevel = new CDbLevel();
	if (!this->pLevel) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pLevel->Load(this->pRoom->dwLevelID);
	if (!bSuccess) goto Cleanup;

	//Load hold associated with level.
	this->pHold = new CDbHold();
	if (!this->pHold) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pHold->Load(this->pLevel->dwHoldID);
	if (!bSuccess) goto Cleanup;
	
	//Set entrance to the main level entrance.
	this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);

	//Set room start vars.
	this->pPlayer->wX = this->pPlayer->wPrevX = wX;
	this->pPlayer->wY = this->pPlayer->wPrevY = wY;
	this->pPlayer->SetOrientation(wO);
	this->pPlayer->wIdentity = wIdentity;
	this->pPlayer->bSwordOff = bSwordOff;

	//The caller is responsible for having invoked this previously.
	//InitRPGStats(this->pPlayer->st);

	SetRoomStartToPlayer();

	SetPlayerToRoomStart();

	SetMembersAfterRoomLoad(CueEvents);
	ProcessCommand_EndOfTurnEventHandling(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*****************************************************************************
bool CCurrentGame::LoadFromSavedGame(
//Loads current game from a saved game.
//
//Params:
	const UINT dwSavedGameID, //(in) Identifies saved game to load from.
	CCueEvents &CueEvents,     //(out)  Cue events generated by swordsman's last 
								//    step in the room (which may be the first step).
	bool bRestoreAtRoomStart,  //(in)   If true, current game will be loaded to
								//    beginning of room in saved game.  If false (default),
								//    current game will be loaded to the exact room
								//    state specified in the saved game.
	const bool bNoSaves)    //whether DB saves should be prevented [default=false]
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromSavedGame");

	try {

	this->bNoSaves = bNoSaves;
	LoadPrep();
	CueEvents.Clear(); //ensure object starts empty

	//Load the saved game.
	if (!CDbSavedGame::Load(dwSavedGameID))
		throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the room.
	this->pRoom = CDbSavedGame::GetRoom();
	if (!this->pRoom) throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the level.
	this->pLevel = this->pRoom->GetLevel();
	if (!this->pLevel) throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the hold.
	this->pHold = this->pLevel->GetHold();
	if (!this->pHold) throw CException("CCurrentGame::LoadFromSavedGame");

	CDbSavedGame::setMonstersCurrentGame(this);

	//Set room start vars.
	this->pPlayer->wX = this->pPlayer->wPrevX = CDbSavedGame::wStartRoomX;
	this->pPlayer->wY = this->pPlayer->wPrevY = CDbSavedGame::wStartRoomY;
	this->pPlayer->SetOrientation(CDbSavedGame::wStartRoomO);
	SetPlayerRole(CDbSavedGame::wStartRoomAppearance);
	this->pPlayer->bSwordOff = CDbSavedGame::bStartRoomSwordOff;
	UnpackData(this->stats);
	SetRoomStartToPlayer();

	const bool bAtRoomStart = bRestoreAtRoomStart || this->Commands.Empty();

	//Determine level entrance where player is.
	//This is currently used only to show level entrance texts and entrance warp cheat.
	this->pEntrance = NULL;
	if (bAtRoomStart)
	{
		//At a level entrance position that should be shown?
		CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
				this->pPlayer->wX, this->pPlayer->wY);
		if (pEntrance)
			this->pEntrance = pEntrance;
	}
	if (!this->pEntrance)
		this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);

	//Put room in correct beginning state and get cue events for the 
	//last step the player has taken.
	RetrieveExploredRoomData(*this->pRoom);
	if (bAtRoomStart)
	{
		//Cue events come from first step into the room.
		SetMembersAfterRoomLoad(CueEvents, false);
		ProcessCommand_EndOfTurnEventHandling(CueEvents);
	} else {
		//Cue events come from processing of last command below.
		//Ignore cue events from first step into the room.
		CCueEvents IgnoredCueEvents;
		SetMembersAfterRoomLoad(IgnoredCueEvents, false);
		ProcessCommand_EndOfTurnEventHandling(IgnoredCueEvents);

		//Play through any commands from the saved game.
		//Truncate any commands that cannot be played back.
		if (!PlayCommands(this->Commands.Count(), CueEvents, true))
		{
			//There was an error encountered during playback.
#ifdef _DEBUG
			char szMsg[80];
			sprintf(szMsg, "Commands in saved game ID#%u could not be played back." NEWLINE,
					dwSavedGameID);
			CFiles Files;
			Files.AppendErrorLog(szMsg);
#endif
		}

		if (!this->bIsGameActive)
		{
			//Playing back the saved game didn't work as expected.
			//Undo to the last command when the game was active.
			UndoCommand(CueEvents);
		}

		UpdatePrevCoords();  //start everything moving from current positions
	}

	} //try
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CCurrentGame::LoadNewRoomForExit(
//Loads room adjacent to the current room in the direction of the swordsman's
//exit of the current room.  Swordsman coords are also updated so that he wraps to
//other side in new room.
//
//Params:
	const UINT wExitO,      //(in)   Direction swordsman is exiting.
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first step 
							//    into the room.
{
	switch (wExitO)
	{
		case N:
			if (!LoadNorthRoom())
				return false;
		break;
		case S:
			if (!LoadSouthRoom())
				return false;
		break;
		case W:
			if (!LoadWestRoom())
				return false;
		break;
		case E:
			if (!LoadEastRoom())
				return false;
		break;
		default:
			ASSERT(!"Bad orientation value.");
		return false;
	}

	//Activate and synch loaded room to its saved state.
	this->pRoom->SetCurrentGame(this);
	CDbSavedGame::SetMonsterListAtRoomStart();
	RetrieveExploredRoomData(*this->pRoom);

	//Set start room members.
	SetRoomStartToPlayer();
	SetPlayerToRoomStart(); //reset some vars so saving game works properly

	SetMembersAfterRoomLoad(CueEvents);

	CueEvents.Add(CID_ExitRoom, new CAttachableWrapper<UINT>(wExitO), true);

	return true;
}

//*****************************************************************************
void CCurrentGame::LoadPrep(
//What needs to be done before loading (i.e. changing) a current game.
//
//Params:
	const bool bNewGame)  //(in)   whether new game is starting [default=true]
{
/*
	if (bNewGame && !this->bNoSaves)
	{
		//On new game, end and save a demo if in the middle of recording.
		//Otherwise, keep recording a multi-room(level) demo.
		if (IsDemoRecording() && !EndDemoRecording())
		{
			CFiles Files;
			Files.AppendErrorLog("Failed to save a demo when recording ended." NEWLINE);
		}
	}
*/

	//Unload an already loaded current game if there is one.
	{
//		const UINT dwAutoSaveOptionsB4 = this->dwAutoSaveOptions;
//		const DEMO_REC_INFO demoInfo = this->DemoRecInfo;
//		const bool bDemoRecording = this->bIsDemoRecording;
		const bool bQuickCombat = this->bQuickCombat;
		const PlayerStats st = this->pPlayer->st;
		Clear(bNewGame);
//		this->dwAutoSaveOptions = dwAutoSaveOptionsB4;
		if (!bNewGame)
		{
//			this->DemoRecInfo = demoInfo;
//			this->bIsDemoRecording = bDemoRecording;
		}
		this->bQuickCombat = bQuickCombat;
		this->pPlayer->st = st;
	}
}

//***************************************************************************************
bool CCurrentGame::MayUseAccessory() const
//Returns: whether the current accessory may be used by the player
{
	CSwordsman& p = *this->pPlayer;
	if (!p.IsInRoom())
		return false;

	CDbRoom& room = *this->pRoom;
	switch (p.st.accessory)
	{
		case InvisibilityPotion:
			return !p.IsInvisible();
		case SpeedPotion:
			return !p.IsHasted();

		case HandBomb:
			return true;

		//May break down any wall tile.
		case PickAxe:
		{
			const UINT wO = p.wO;
			const UINT destX = p.wX + nGetOX(wO);
			const UINT destY = p.wY + nGetOY(wO);
			if (!room.IsValidColRow(destX, destY))
				return false;

			const UINT oTile = room.GetOSquare(destX, destY);
			return bIsWall(oTile) || bIsCrumblyWall(oTile);
		}

		//May open any kind of door.
		case PortableOrb:
		{
			const UINT wO = p.wO;
			const UINT destX = p.wX + nGetOX(wO);
			const UINT destY = p.wY + nGetOY(wO);
			if (!room.IsValidColRow(destX, destY))
				return false;

			const UINT oTile = room.GetOSquare(destX, destY);
			return bIsDoor(oTile);
		}

		//Destination tiles must not contain a general obstacle.
		case WarpToken:
		{
			const UINT reflectX = room.wRoomCols - p.wX - 1;
			const UINT reflectY = room.wRoomRows - p.wY - 1;
			ASSERT(room.IsValidColRow(reflectX, reflectY));
			return !room.DoesSquareContainPlayerObstacle(
					 reflectX, reflectY, NO_ORIENTATION,
					 bIsElevatedTile(room.GetOSquare(p.wX, p.wY)));
		}
		case WallWalking:
		{
			const UINT nextX = p.wX + nGetOX(p.wO);
			const UINT nextY = p.wY + nGetOY(p.wO);

			const UINT destX = nextX + nGetOX(p.wO);
			const UINT destY = nextY + nGetOY(p.wO);
			if (!room.IsValidColRow(destX, destY))
				return false;

			//Can't walk through pit or water.
			ASSERT(room.IsValidColRow(nextX, nextY));
			const UINT nextOTile = room.GetOSquare(nextX, nextY);
			if (bIsPit(nextOTile) || bIsWater(nextOTile))
				return false;

			return !room.DoesSquareContainPlayerObstacle(
					 destX, destY, NO_ORIENTATION,
					 bIsElevatedTile(room.GetOSquare(p.wX, p.wY)));
		}

		case NoAccessory:
			return false;
		default:
			//Custom accessories may be used if the script permits it.
			return bIsCustomEquipment(p.st.accessory);
	}
}

//***************************************************************************************
bool CCurrentGame::MayUseItem(const UINT type) const
//Returns: whether the inventory type may be used by the player
{
	//May not use items when not in the room,
	//there are questions to answer,
	//or a combat or cut scene is in progress.
	if (!this->pPlayer->IsInRoom())
		return false;
	if (this->UnansweredQuestions.size())
		return false;
	if (InCombat())
		return false;
	if (this->dwCutScene)
		return false;

	switch (type)
	{
		//Custom weapons and armor may be used.  Base types may not.
		case ScriptFlag::Weapon:
			if (this->pPlayer->IsSwordDisabled())
				return false;
			return bIsCustomEquipment(this->pPlayer->st.sword);
		case ScriptFlag::Armor:
			if (IsPlayerShieldDisabled())
				return false;
			return bIsCustomEquipment(this->pPlayer->st.shield);

		case ScriptFlag::Accessory:
			if (IsPlayerAccessoryDisabled())
				return false;
			return MayUseAccessory();

		//Special architect-defined command scripts may be executed
		//any time the above general item use conditions are satisfied,
		//as long as a script has not disabled the ability.
		case ScriptFlag::Command:
			return !this->pPlayer->bCommandOff;

		default: return false; //unrecognized inventory type
	}
}

//***************************************************************************************
bool CCurrentGame::UseAccessory(CCueEvents &CueEvents)
//Alter game state according to the accessory used.
//
//Returns: whether the player moved as a result of using the accessory
{
	if (!MayUseAccessory())
		return false;

	bool bMoved = false;
	const UINT accessory = this->pPlayer->st.accessory;
	switch (accessory)
	{
		case InvisibilityPotion:
			//Drink invisibility potion.
			ASSERT(!this->pPlayer->IsInvisible());
			this->pPlayer->bInvisible = true;
			CueEvents.Add(CID_DrankPotion);
//			CueEvents.Add(CID_AccessoryUsed, new CAttachableWrapper<UINT>(accessory), true);
		break;
		case SpeedPotion:
			//Drink speed potion.
			ASSERT(!this->pPlayer->IsHasted());
			this->pPlayer->bHasted = true;
			CueEvents.Add(CID_DrankPotion);
//			CueEvents.Add(CID_AccessoryUsed, new CAttachableWrapper<UINT>(accessory), true);
		break;

		case HandBomb:
		{
			//Generate a 3x3 explosion that doesn't affect the player tile.
			const UINT wX = this->pPlayer->wX, wY = this->pPlayer->wY;
			CCoordSet explosion; //what tiles are affected by the explosion
			CCoordStack bombs, coords;
			for (int y = -1; y <= 1; ++y)
				for (int x = -1; x <= 1; ++x)
				{
					this->pRoom->ExpandExplosion(CueEvents, coords, wX, wY,
							wX + x, wY + y, bombs, explosion);
				}

			//Now process the effects of the explosion.
			for (CCoordSet::const_iterator exp=explosion.begin(); exp!=explosion.end(); ++exp)
				this->pRoom->ProcessExplosionSquare(CueEvents, exp->wX, exp->wY,
					(exp->wX != wX) || (exp->wY != wY)); //explosion does not hurt self, so don't make one at the origin

			CueEvents.Add(CID_BombExploded, new CMoveCoord(wX, wY, 0), true);

			//If bombs were set off, explode them now.
			//These could harm the player.
			if (bombs.GetSize())
				this->pRoom->BombExplode(CueEvents, bombs);

			this->pRoom->ConvertUnstableTar(CueEvents);
		}
		break;

		case PickAxe:
		{
			//Knock down any wall in front of the player.
			const UINT wO = this->pPlayer->wO;
			const UINT destX = this->pPlayer->wX + nGetOX(wO);
			const UINT destY = this->pPlayer->wY + nGetOY(wO);
			const UINT oTile = this->pRoom->GetOSquare(destX, destY);
			ASSERT(bIsWall(oTile) || bIsCrumblyWall(oTile));
			const UINT replacementTile = bIsCrumblyWall(oTile) ?
					this->pRoom->coveredOSquares.GetAt(destX, destY) :
					T_FLOOR;
			this->pRoom->Plot(destX, destY, replacementTile);
			CueEvents.Add(CID_CrumblyWallDestroyed, new CMoveCoord(destX, destY, wO), true); 
		}
		break;
		case PortableOrb:
		{
			//Open any kind of door in front of the player.
			const UINT wO = this->pPlayer->wO;
			const UINT destX = this->pPlayer->wX + nGetOX(wO);
			const UINT destY = this->pPlayer->wY + nGetOY(wO);
			const UINT oTile = this->pRoom->GetOSquare(destX, destY);
			ASSERT(bIsDoor(oTile));
			this->pRoom->OpenDoor(destX, destY);
			CueEvents.Add(CID_PortableOrbActivated);
		}
		break;

		case WarpToken:
		{
			//Warp to reflected room coord.
			const UINT reflectX = this->pRoom->wRoomCols - this->pPlayer->wX - 1;
			const UINT reflectY = this->pRoom->wRoomRows - this->pPlayer->wY - 1;
			bMoved = SetPlayer(reflectX, reflectY);
			CueEvents.Add(CID_AccessoryUsed, new CAttachableWrapper<UINT>(accessory), true);
		}
		break;
		case WallWalking:
		{
			//Warp ahead two tiles in the direction faced.
			const UINT destX = this->pPlayer->wX + nGetOX(this->pPlayer->wO) * 2;
			const UINT destY = this->pPlayer->wY + nGetOY(this->pPlayer->wO) * 2;
			bMoved = SetPlayer(destX, destY);
			CueEvents.Add(CID_AccessoryUsed, new CAttachableWrapper<UINT>(accessory), true);
		}
		break;

		default:
		{
			//Process a custom accessory script.
			CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Accessory);
			if (pCharacter) {
				const UINT wX = this->pPlayer->wX, wY = this->pPlayer->wY;
				pCharacter->ProcessAfterUse(CueEvents);
				if (this->pPlayer->wX != wX || this->pPlayer->wY != wY)
					return true; //show this movement; don't override in ProcessPlayer:MakeMove
			}
		}
		//return in order to not expend the item below
		return false; //assume player doesn't move
	}

	this->pPlayer->st.accessory = NoAccessory;
	return bMoved;
}

//***************************************************************************************
bool CCurrentGame::PlayCommands(
//Play back stored commands to change the game state.
//Assumes that the room has been freshly loaded when replaying from turn zero.
//
//Params:
	const UINT wCommandCount,  //(in)   Number of commands to play back.
	CCueEvents &CueEvents,     //(out)  Cue events generated by last processed command.
	const bool bTruncateInvalidCommands)   //(in) delete any commands that cannot be played back [default=false]
//
//Returns:
//True if commands were successfully played without putting the game into an 
//unexpected state, false if not.
{
	ASSERT(wCommandCount <= this->Commands.Count());

	if (!wCommandCount)
		return true; //no commands to be played
	
	//While processing the command list, I don't want to take any actions that
	//will modify the command list.
	FreezeCommands();

	//Replay turns starting from the turn # of the current game state.

	CDbCommands::const_iterator comIter = this->Commands.Get(this->wTurnNo);
	UINT wCommandI, wX=(UINT)-1, wY=(UINT)-1;
	for (wCommandI = this->wTurnNo; wCommandI < wCommandCount; ++wCommandI)
	{
		ASSERT(comIter != this->Commands.end());
		DeleteLeakyCueEvents(CueEvents);
		CueEvents.Clear();

		const int nCommand = static_cast<int>(comIter->command);
		if (bIsComplexCommand(nCommand)) //handle multi-part commands here
			VERIFY(this->Commands.GetData(wX,wY));
		ProcessCommand(nCommand, CueEvents, wX, wY);
				
		//Check for game states that indicate further commands would be invalid.
		//Note: a possible reason for getting these errors is that the current version
		//of the app is not compatible with a game previously saved.
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
		{
			++wCommandI;    //command ending room play executed successfully, but demo stops now
			break;
		}
		if (!this->bIsGameActive) break;
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
			break;

		comIter = this->Commands.GetNext();
	}
	this->pPlayer->wPrevX = this->pPlayer->wX;
	this->pPlayer->wPrevY = this->pPlayer->wY;
	this->pPlayer->wPrevO = this->pPlayer->wO;

	//Allow modification of command list again.
	UnfreezeCommands();

	//If not all commands were processed, optionally truncate commands that
	//cannot be played back.
	if (wCommandI < wCommandCount && bTruncateInvalidCommands)
		this->Commands.Truncate(wCommandI);

	//Successful return if I processed all the commands.
	return wCommandI == wCommandCount;
}

//*****************************************************************************
void CCurrentGame::ProcessCommandSetVar(
//Called when some predefined variables are changed,
//e.g. via CMD_SETVAR command and CCharacter::SetPredefinedVar.
//It is used to alter the state values of the current game directly.
	CCueEvents& CueEvents,
	const UINT itemID, //ScriptVars Predefined vars enum
	UINT newVal)
{
	ASSERT(this->pPlayer);

	//Check for special things that need to happen as a result of altering
	//certain vars.
	ScriptFlag::EquipmentType equipType = ScriptFlag::NotEquipment;
	switch (itemID)
	{
		default:
			//simple var modification
			this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), newVal);
		return;

		case (UINT)ScriptVars::P_HP:
			if (int(newVal) < 1)
				newVal = 1; //constrain HP to preserve life
			this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), newVal);
		return;

		case (UINT)ScriptVars::P_SWORD: equipType = ScriptFlag::Weapon; break;
		case (UINT)ScriptVars::P_SHIELD: equipType = ScriptFlag::Armor; break;
		case (UINT)ScriptVars::P_ACCESSORY: equipType = ScriptFlag::Accessory; break;

		case (UINT)ScriptVars::P_SPEED:
		case (UINT)ScriptVars::P_TOTALMOVES:
		case (UINT)ScriptVars::P_TOTALTIME:
			//cannot alter
			return;
	}

	ASSERT(equipType != ScriptFlag::NotEquipment); //should have been set above

	//Make change to inventory.
	changingInventory(CueEvents, equipType, newVal);

	//Due to possible complex custom scripting that applying the new inventory item
	//could have invoked, we might not actually still have the expected item
	//in the inventory slot.
	//So, we will set the state var based on what type of equipment actually is
	//in the player's possession at this point.
	if (!bIsCustomEquipment(newVal))
	{
		//If setting non-custom equipment, the expected item will be in inventory at this point
		//so we don't need to make any special checks.
		this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), newVal);
		return;
	}

	//Check that we actually have the type of custom equipment we expect in this slot.
	CCharacter* pCharacter = getCustomEquipment(equipType);
	if (pCharacter)
	{
		//We do have *some* custom character set to this item slot.
		//Use its value for our current equipment state.
		//this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), pCharacter->dwScriptID);
		this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), newVal);
	} else {
		//No we don't -- so if the player's item slot state var is currently set to
		//a custom item, we need to reset it because we're not actually carrying
		//it now.
		const UINT oldItemID = this->pPlayer->st.getVar(ScriptVars::Predefined(itemID));
		if (bIsCustomEquipment(oldItemID))
		{
			//Set to the empty item slot for this slot type.
			UINT emptyVal = 0;
			switch (itemID)
			{
				case (UINT)ScriptVars::P_SWORD: emptyVal = NoSword; break;
				case (UINT)ScriptVars::P_SHIELD: emptyVal = NoShield; break;
				case (UINT)ScriptVars::P_ACCESSORY: emptyVal = NoAccessory; break;
				default: ASSERT(!"Unexpected item type"); break;
			}
			this->pPlayer->st.setVar(ScriptVars::Predefined(itemID), emptyVal);
			return;
		}

		//The player's slot is not set to a custom item, so we'll just leave it
		//at whatever it currently is without altering it.
	}
}

//*****************************************************************************
void CCurrentGame::ProcessCommand(
//Processes a game command, causing game data and current room to be updated
//in response.  Display or sound should be handled by caller if appropriate.
//
//Params:
	int nCommand,        //(in)   Game command.
	CCueEvents &CueEvents,  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
	const UINT wX, const UINT wY)	//(in) [default=(-1,-1)]
{
	//Caller should not be trying to process new commands after the game is
	//inactive.  Before doing so, caller will need to reload the room in some way.
	ASSERT(this->bIsGameActive);

	//state information that should not carry over across turns
	ASSERT(this->pendingTokenActivations.empty());
	ASSERT(this->simulSwordHits.empty());
	ASSERT(this->possibleTarStabs.empty());
	ASSERT(this->changingInventoryTypes.empty());
	ASSERT(!this->pPendingPriorLocation);

	//Reset relative movement for the current turn.
	UpdatePrevPlatformCoords();

//	const UINT dwStart = GetTicks();
	
	this->pPlayer->bHasTeleported = false;

	const bool bPlayerIsAnsweringQuestion = this->UnansweredQuestions.size() != 0;

	//Add this command to list of commands for the room.
	//Note: Private data pointers found in CueEvents are guaranteed to be valid
	//until the CCurrentGame instance that originally returned the CueEvents goes out
	//of scope or the ProcessCommand() method is called again on the same instance.  
	CueEvents.Clear();
	this->pRoom->ClearPlotHistory();

	if (this->Commands.IsFrozen())
	{
		//While replaying a command sequence, synthesize combat tick commands until combat is done.
		if (nCommand != CMD_ADVANCE_COMBAT)
		{
			while (InCombat())
			{
				ProcessCommand(CMD_ADVANCE_COMBAT, CueEvents);
				if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
					return; //something unexpected happened -- don't execute any other logic for this turn
				CueEvents.Clear(); //don't keep these events around to display, since the next move is replaying now
			}
		}
	} else {
		//During play, ensure that the commands coming in are valid.
		//Ignore any commands that are invalid for the current play state.
		if (InCombat())
		{
			//While in combat, keep persistent events going.
			this->pRoom->BurnFuseEvents(CueEvents);

			if (nCommand != CMD_ADVANCE_COMBAT)
				return;

			//Don't add combat ticks to command sequence.
			//These will be synthesized on playback.
			//this->Commands.Add(nCommand, this->dwCutScene);
		}
		else if (this->dwCutScene)
		{
			//Only accept explicit time-advancement commands from the front end during cut scenes
			//...except for answering questions put forth during the cut scene
			if (bPlayerIsAnsweringQuestion && bIsQuestionResponseCommand(nCommand))
			{
				this->Commands.Add(nCommand);
				if (nCommand == CMD_ANSWER) //store selected answer as a second command item
					this->Commands.AddData(wX, wY);
			} else {
				if (nCommand != CMD_ADVANCE_CUTSCENE)
					return;
				nCommand = CMD_WAIT;
				this->Commands.Add(nCommand, this->dwCutScene);
			}
		}
		else //if (!this->pPlayer->wPlacingDoubleType)
		{
			//If player is not being shown, shouldn't be adding any commands.
			//(Hmmm...I can't remember how this can happen when not in a cut scene.)
			if (!this->pPlayer->IsInRoom())
				return;

			//When the player has requested an inventory item be used, verify it is usable.
			//If not, the command is not executed.
			if (bIsInventoryCommand(nCommand))
			{
				ScriptFlag::EquipmentType type;
				switch (nCommand)
				{
					case CMD_USE_WEAPON: type = ScriptFlag::Weapon; break;
					case CMD_USE_ARMOR: type = ScriptFlag::Armor; break;
					case CMD_USE_ACCESSORY: type = ScriptFlag::Accessory; break;
					case CMD_EXEC_COMMAND: type = ScriptFlag::Command; break;
					default: type = ScriptFlag::NotEquipment; break;
				}
				if (!MayUseItem(type))
				{
					CueEvents.Add(CID_CantUseAccessory);
					return;
				}
			}

			this->Commands.Add(nCommand);
			if (bIsComplexCommand(nCommand)) //store data parameters as a second command item
				this->Commands.AddData(wX, wY);
		}	//else: handle command addition below in ProcessDoublePlacement //not used in RPG anyway
	}

	//Set a state variable and do nothing else.
	if (nCommand == CMD_SETVAR)
	{
		ProcessCommandSetVar(CueEvents, wX, wY);
		return;
	}

	//Increment the turn#.
	if (//!this->pPlayer->wPlacingDoubleType && 
		nCommand != CMD_ADVANCE_COMBAT)
	{
		++this->wTurnNo;
		++this->pPlayer->st.totalMoves;
		ASSERT(this->pPlayer->st.totalMoves > 0);
	}

/*
	//Switch to another clone.
	if (nCommand == CMD_CLONE)
	{
		//Only allow when not in process of placing a double.
		if (!this->pPlayer->wPlacingDoubleType)
		{
			SwitchToCloneAt(wX,wY);
			this->pRoom->ProcessTurn(CueEvents, false);
		}
		return;
	}
*/

//	const UINT wOriginalMonsterCount = this->pRoom->wMonsterCount;
	this->bContinueCutScene = false;
	const bool bInCombat = InCombat();

	//If there are any unanswered questions, process them.
	if (bPlayerIsAnsweringQuestion && !bInCombat)
	{
		//If player has questions to answer during a cut scene, wait until player responds.
		if (this->dwCutScene && nCommand == CMD_WAIT)
			return;

		//Decode command data.
		const int nArgument = nCommand == CMD_ANSWER ? wX*256 + wY : nCommand;
		ProcessUnansweredQuestions(nArgument, this->UnansweredQuestions, CueEvents);

		//After answering all questions, allow NPCs to execute non turn-expending commands.
		if (this->UnansweredQuestions.empty())
		{
			ProcessMoveFreeScripts(CueEvents, this->pRoom->pFirstMonster);
			ProcessMoveFreeScripts(CueEvents, CDbSavedGame::pMonsterList); //global scripts

			this->pRoom->ProcessTurn(CueEvents, false);
		}
	} else {
/*
		if (this->pPlayer->wPlacingDoubleType)
			ProcessDoublePlacement(nCommand, CueEvents, wX, wY);
		else
*/
		{
			ASSERT(!bIsComplexCommand(nCommand));

			if (bInCombat)
			{
				AdvanceCombat(CueEvents);

				//Only logic that doesn't depend on how many rounds a combat
				//might be fought may be executed on this turn.
				if (!this->simulSwordHits.empty())
				{
					//Destroy tar tile stabbed and defeated in combat.
					ProcessSimultaneousSwordHits(CueEvents);
				}

				//Calling this will handle releasing any pressure plate previously
				//held down by the deceased or shortened monster.
				if (CueEvents.HasOccurred(CID_MonsterDiedFromStab) ||
						CueEvents.HasOccurred(CID_SnakeDiedFromTruncation) ||
						CueEvents.HasOccurred(CID_MonsterPieceStabbed) ||
						CueEvents.HasOccurred(CID_NPC_Defeated) ||
						!InCombat()) //e.g., on conclusion of consecutive no-damage hits
					this->pRoom->ProcessTurn(CueEvents, false);

				//WARNING -- Can't process NPC scripts during combat rounds, because if
				//a quick combat is executed, combat will only take one round instead
				//of several.  Any game logic that might play out differently
				//depending on how many advance combat commands are executed is forbidden.
				//
				//NPC scripts can execute move-free commands to catch events of the battle.
				//ProcessMoveFreeScripts(CueEvents, this->pRoom->pFirstMonster);

				//However, if we're in a scripted cut scene, let it continue once combat is done.
				if (this->dwCutScene)
					this->bContinueCutScene = true;
			} else {
				//No combat should be occurring at this point.
				ASSERT(!InCombat());
				delete this->pCombat;
				this->pCombat = NULL;
				this->pBlockedSwordHit = NULL;

				//These flags are reset for monster behavior at the start of each "real" turn before the player moves.
				this->pRoom->ResetTurnFlags();

				//Player takes a turn when in the room.
				++this->wPlayerTurn;    //do first -- increment even when CIDA_PlayerLeftRoom and before demo is saved
				if (this->pPlayer->IsInRoom())
					ProcessPlayer(nCommand, CueEvents);
			}

/*
			//If player tried to exit room when locked, then unwind the move in
			//progress as if it didn't happen, except for receiving the exit locked event.
			if (CueEvents.HasOccurred(CID_RoomExitLocked))
			{
				//Room exit should never be locked during move playback.
				ASSERT(!this->Commands.IsFrozen());

				//It should be impossible for player to exit a room during a cutscene.
				ASSERT(!this->dwCutScene);

				//Undo move counts.
				ASSERT(this->wPlayerTurn);
				--this->wPlayerTurn;
				ASSERT(this->wTurnNo);
				--this->wTurnNo;

//				ASSERT(this->pPlayer->st.totalMoves);
//				--this->pPlayer->st.totalMoves;
				if (!this->Commands.IsFrozen())
					this->Commands.RemoveLast();

				return;
			}
*/

			if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
			{
				//If play in room stopped, then room processing won't take place
				//and outstanding data must be cleaned up here.
				CPlatform::clearFallTiles();
				this->simulSwordHits.clear();
				this->possibleTarStabs.clear();
				UpdatePrevCoords(); //monsters are no longer moving from previous position
			}
			else if (nCommand != CMD_ADVANCE_COMBAT)
			{
				//After player's turn, everything else in room takes a turn.
				this->bHalfTurn = this->pPlayer->IsHasted() && !this->bHalfTurn;

				ProcessMonsters(nCommand, CueEvents);

				//Run global scripts.
				ProcessScripts(nCommand, CueEvents, CDbSavedGame::pMonsterList);

				//If a monster caused a room exit, then process nothing else for the current turn.
				if (CueEvents.HasOccurred(CID_ExitRoom))
					return;

				//Check for stuff falling as a result of monster moves now.
				if (CPlatform::fallTilesPending())
					CPlatform::checkForFalling(this->pRoom, CueEvents);

				ProcessSimultaneousSwordHits(CueEvents);  //destroy simultaneously-stabbed tar

				this->pRoom->ProcessTurn(CueEvents, !this->bHalfTurn);

				//If goo temporarily took away sword, but then something covered it up
				//and sword came back, process a sword hit now.
				//NOTE: It won't be synched with other sword hits, but that's the way it goes.
				if (bEntityHasSword(this->pPlayer->GetIdentity()) && !this->pPlayer->HasSword())
				{
					SetPlayerSwordSheathed();
					if (this->pPlayer->HasSword())
					{
						const UINT wSX = this->pPlayer->GetSwordX();
						const UINT wSY = this->pPlayer->GetSwordY();
						ProcessSwordHit(wSX, wSY, CueEvents);
						ProcessSimultaneousSwordHits(CueEvents);

						//In case sword hit changed something, must check room stuff again.
						this->pRoom->ProcessTurn(CueEvents, false);
					}
				}

				//If monster being dueled to destroy tarstuff has somehow been killed this turn (e.g., by explosion)
				//and there is no other monster to fight on the tarstuff,
				//then the stabbed tarstuff needs to be removed at this point.
				bool bRemovedTarstuff = false;
				for (vector<TarstuffStab>::const_iterator it=this->possibleTarStabs.begin();
						it!=possibleTarStabs.end(); ++it)
				{
					const CMoveCoord& stabCoord = it->moveCoord;
					if (!it->pTarstuffMonster->IsAlive())
					{
						CMonster *pMother = this->pRoom->GetMotherConnectedToTarTile(stabCoord.wX, stabCoord.wY);
						if (pMother) {
							//Start a fight with a remaining live mother.
							delete this->pCombat;
							this->pCombat = NULL;
							InitiateCombat(CueEvents, pMother, true, 0, 0, stabCoord.wX, stabCoord.wY, true);
						} else {
							if (this->pRoom->StabTar(stabCoord.wX, stabCoord.wY, CueEvents, true, stabCoord.wO))
								bRemovedTarstuff = true;
						}
					}
				}
				this->possibleTarStabs.clear();
				if (bRemovedTarstuff) //Sword hit changed something, so must check room stuff again.
					this->pRoom->ProcessTurn(CueEvents, false);

				SetPlayerMood(CueEvents);
			}
		}
	}

	ProcessCommand_EndOfTurnEventHandling(CueEvents);


	if (!CueEvents.HasOccurred(CID_ExitRoom)) //on exit, this will have already been handled for the new room
		AmbientSoundTracking(CueEvents);

	//Reset the combat structure any time a turn has passed without combat ending,
	//beginning, or continuing (for front-end display purposes).
	if (!bInCombat && !CueEvents.HasOccurred(CID_MonsterEngaged) && !InCombat())
	{
		delete this->pCombat;
		this->pCombat = NULL;
	}

	//Should never have any of these left unprocessed at the end of a turn.
	ASSERT(!CPlatform::fallTilesPending());
	ASSERT(this->pendingTokenActivations.empty());
	ASSERT(this->simulSwordHits.empty());
	ASSERT(this->possibleTarStabs.empty());
	ASSERT(this->changingInventoryTypes.empty());

/*
	//Make a queue of periodic game snapshots that can be retrieved to reduce
	//in-game rewind/replay time.
	const UINT dwElapsed = (GetTicks() - dwStart) + 1;
	this->dwComputationTime += dwElapsed;
	if (this->dwComputationTime >= this->dwComputationTimePerSnapshot &&
			!InCombat() &&       //don't record snapshot in the middle of a fight
			this->bIsGameActive) //don't record snapshot if play has ended
	{
		this->dwComputationTime = 0; //reset before saving snapshot
		CCurrentGame *pNewSnapshot = new CCurrentGame(*this);
		if (pNewSnapshot)
		{
			pNewSnapshot->pSnapshotGame = this->pSnapshotGame;
			this->pSnapshotGame = pNewSnapshot;
		}
	}
*/

	//If a combat was engaged via sword hit when the player can't harm
	//the monster being attacked but the monster could harm the player,
	//then this is considered an invalid move and must be rewound.
	//Rationale: if the player had sufficient attack power, then the player
	//would be harmed by this combat.  It is paradoxical that a player with
	//low attack power can make a move that doesn't hurt, whereas a player with
	//high attack power would be hurt by this move.  Instead, we choose to
	//forbid it entirely.
	if (this->pBlockedSwordHit)
	{
		static CMoveCoord coord;
		coord.wX = this->pBlockedSwordHit->wX;
		coord.wY = this->pBlockedSwordHit->wY;
		if (!this->Commands.IsFrozen())
			UndoCommand(CueEvents); //can't undo when move sequence is frozen (avoid infinite recursion)
		CueEvents.Clear(); //don't show events from previous turn again
		CueEvents.Add(CID_InvalidAttackMove, &coord); //add an event to let the front-end know what happened
	}
}

//Checks that happen at the end of ProcessCommand.
//This method also needs to be called anytime a new room is loaded, before commands are (re)played.
//This includes when starting a new hold/level/room, on room restart (that includes move undo), and loading game.
void CCurrentGame::ProcessCommand_EndOfTurnEventHandling(CCueEvents& CueEvents) //(in/out)
{
	ProcessTokenActivations(CueEvents);

	//Check for new questions that were asked.  Put them in a list of questions
	//for which answers will be expected on subsequent calls.
	AddQuestionsToList(CueEvents, this->UnansweredQuestions);

	//Did player die this turn?
	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
	{
		//The player died -- remember that the game is now inactive.
		this->pPlayer->bAlive = false;
		this->bIsGameActive = false;
		//		++this->dwLevelDeaths;
		//		Manual and automatic recording of death demos handled in front end.
	}

	/*
		if (bIsMovementCommand(nCommand))
			QueryCheckpoint(CueEvents, this->pPlayer->wX, this->pPlayer->wY);
		if (CueEvents.HasOccurred(CID_CheckpointActivated))
		{
			CCoord *pCoord = DYN_CAST(CCoord*, CAttachableObject*,
					CueEvents.GetFirstPrivateData(CID_CheckpointActivated));
			ASSERT(pCoord);
			this->wLastCheckpointX = pCoord->wX;
			this->wLastCheckpointY = pCoord->wY;

			this->checkpointTurns.push_back(this->wTurnNo);
			//If player is still alive after checkpoint was stepped on:
			if (!this->Commands.IsFrozen() && this->bIsGameActive)
				SaveToCheckpoint();
		} else {
			//If checkpoint wasn't touched this turn, allow saving to it starting next turn.
			if (this->pRoom->IsValidColRow(this->wLastCheckpointX, this->wLastCheckpointY) &&
					(this->wLastCheckpointX != this->pPlayer->wX ||
						this->wLastCheckpointY != this->pPlayer->wY) &&
					!this->pRoom->IsMonsterOfTypeAt(M_MIMIC, this->wLastCheckpointX, this->wLastCheckpointY))
				this->wLastCheckpointX = this->wLastCheckpointY = static_cast<UINT>(-1);
		}
		//If there were monsters, but not any more, then remove green doors.
		if (CueEvents.HasOccurred(CID_AllMonstersKilled) && !this->pRoom->bGreenDoorsOpened)
			if (ToggleGreenDoors())
				//Adding this event with true flag indicates green doors toggled?
				CueEvents.Add(CID_AllMonstersKilled, new CAttachableWrapper<bool>(true), true);
	*/
	//Return cue event for plots if any plots were made.  This check needs
	//to go after any code that could call pRoom->Plot().
	if (this->pRoom->PlotsMade.size())
		CueEvents.Add(CID_Plots, &(this->pRoom->PlotsMade));

	//During active moves, check for swords colliding.
	if (//!this->pPlayer->wPlacingDoubleType &&
		this->pRoom->SwordfightCheck())
		CueEvents.Add(CID_Swordfight);

	//Call once all cue events could have fired.
	if (this->bIsGameActive) //don't need to check when game is no longer in play (incl. transitioning to a new level)
	{
		this->pRoom->CharactersCheckForCueEvents(CueEvents, this->pRoom->pFirstMonster);
		this->pRoom->CharactersCheckForCueEvents(CueEvents, CDbSavedGame::pMonsterList); //global scripts
	}

	//Cut scene updates.
	if (!this->bContinueCutScene)
		this->dwCutScene = 0;

	//Player should always be visible while cut scene is not playing.
	if (!this->pPlayer->IsInRoom() && !this->dwCutScene)
		SetPlayerRole(defaultPlayerType()); //place player in room now as default type

/*
	//Update path maps to NPC Beethro.
	if (this->pPlayer->wAppearance != M_BEETHRO)
	{
		UINT wSX, wSY;
		if (GetSwordsman(wSX, wSY))
			this->pRoom->SetPathMapsTarget(wSX, wSY);
	}
*/
}

//*****************************************************************************
void CCurrentGame::ProcessMoveFreeScripts(CCueEvents& CueEvents, CMonster* pMonsterList)
//Execute NPC script commands that don't require a turn to execute.
{
	this->bExecuteNoMoveCommands = true;
	ProcessScripts(CMD_WAIT, CueEvents, pMonsterList);
	this->bExecuteNoMoveCommands = false;
}

//*****************************************************************************
void CCurrentGame::ProcessScripts(int nCommand, CCueEvents& CueEvents, CMonster* pMonsterList)
//Process a turn of all NPC scripts in this list.
{
	CMonster *pMonster = pMonsterList;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pMonster->Process(nCommand, CueEvents);
			if (pCharacter->bScriptDone)
				this->CompletedScriptsPending += pCharacter->dwScriptID;
		}
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
bool CCurrentGame::InCombat() const
//Returns: whether combat is occurring now
{
	if (!this->pCombat)
		return false;

	if (this->pCombat->bEndCombat)
		return false; //don't continue the current fight

//	if (!this->pCombat->PlayerCanHarmQueuedMonster())
//		return false;

	//Is current opponent dead, and there is no queued opponent?
	if (!this->pCombat->pMonster->HP && !this->pCombat->bFightNextMonsterInQueue)
		return false;

	if (!this->pPlayer->st.HP)
		return false;

	return true;
}

//*****************************************************************************
void CCurrentGame::InitiateCombat(
//Attempt to initiate a combat sequence with indicated monster.
//
//Params:
		CCueEvents& CueEvents, CMonster *pMonster,
		const bool bPlayerHitsFirst,  //true if player hits monster with his sword
		const UINT wFromX, const UINT wFromY, //entity from which attack on monster is originating
		const UINT wX, const UINT wY, //which tile of monster was attacked
		const bool bDefeatToStabTarTile) //[default=false]
{
	ASSERT(pMonster);

	if (pMonster->IsPiece())
	{
		CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
		pMonster = pPiece->pMonster;
	}

	//Combat begins if this tile of the monster is vulnerable.
	if (!pMonster->IsDamageableAt(wX,wY))
		return; //no fight begins

	//If this is set to true, it indicates the player has attempted to move his sword
	//onto a monster that cannot be damaged,
	//but could have damaged the player, were the player's attack power sufficient.
	bool bBlockedSwordHit = false;

	if (this->pCombat && this->pCombat->IsCurrent())
	{
		//Player is already set to fight one or more monsters.  Fight this one after the others are defeated.
		if (!this->pCombat->QueueMonster(pMonster, bPlayerHitsFirst, wFromX, wFromY, wX, wY, bDefeatToStabTarTile))
			bBlockedSwordHit = bPlayerHitsFirst && this->pCombat->MonsterCanHarmPlayer(pMonster);
	} else if (this->wTurnNo > 0) { //can't start combat on room entrance
		//The end of SetMembersAfterRoomLoad would clear combat data anyway,
		//and we don't want NPC scripts to catch this event when combat can't actually begin.
		CueEvents.Add(CID_MonsterEngaged, pMonster); //new combat instantiated

		delete this->pCombat;
		this->pCombat = new CCombat(this, pMonster, bPlayerHitsFirst, wFromX, wFromY, wX, wY, bDefeatToStabTarTile);
		bBlockedSwordHit = bPlayerHitsFirst && !this->pCombat->PlayerCanHarmMonster(pMonster) && this->pCombat->MonsterCanHarmPlayer(pMonster);
	}

	if (bBlockedSwordHit)
		this->pBlockedSwordHit = pMonster;
}

//*****************************************************************************
void CCurrentGame::MonsterInitiatesCombat(
//Monster attempts to initiate a combat sequence with the player.
//If the player can't be harmed, do nothing.
//If the monster can't be harmed, the player immediately dies.
//
//Returns: whether combat began
//
//Params:
	CCueEvents& CueEvents,
	CMonster *pMonster)
{
	ASSERT(pMonster);
	ASSERT(!pMonster->IsPiece());

	const UINT wX = pMonster->wX, wY = pMonster->wY;
	if (this->pCombat && this->pCombat->IsCurrent())
	{
		//Player is already set to fight one or more monsters.  Fight this one after the others are defeated.
		if (!this->pCombat->QueueMonster(pMonster, false, wX, wY, wX, wY))
			if (this->pCombat->MonsterCanHarmPlayer(pMonster))
				CueEvents.Add(CID_MonsterKilledPlayer, pMonster);
	} else {
		delete this->pCombat;
		this->pCombat = new CCombat(this, pMonster, false, wX, wY, wX, wY);
		if (this->pCombat->MonsterCanHarmPlayer(pMonster))
		{
			CueEvents.Add(CID_MonsterEngaged, pMonster); //new combat instantiated
			if (!this->pCombat->PlayerCanHarmMonster(pMonster))
				CueEvents.Add(CID_MonsterKilledPlayer, pMonster);
		} 
	}
}

//*****************************************************************************
void CCurrentGame::AdvanceCombat(CCueEvents& CueEvents)
//Advance combat one time unit.
{
	ASSERT(this->pCombat);

	if (!this->pCombat->Advance(CueEvents, this->bQuickCombat))
	{
		//Did monster or player die?
		CMonster *pDefeatedMonster = this->pCombat->pDefeatedMonster;
		if (pDefeatedMonster)
		{
			//Player won -- he stabs the monster on this tile.
			if (this->pCombat->bDefeatToStabTarTile)
			{
				//This fight was performed to remove this tarstuff tile.
				this->simulSwordHits.push_back(CMoveCoord(this->pCombat->wX, this->pCombat->wY, NO_ORIENTATION));

				//Reset the tarstuff mother's HP back to full to allow attacking another tile.
				ASSERT(pDefeatedMonster->wX != this->pCombat->wX || pDefeatedMonster->wY != this->pCombat->wY);
				ASSERT(!pDefeatedMonster->getHP());
				pDefeatedMonster->SetHP();
			} else {
				ProcessMonsterDefeat(CueEvents, pDefeatedMonster, this->pCombat->wX, this->pCombat->wY, GetSwordMovement());
			}

			//If more monsters are queued for fighting,
			//they will be fought automatically the next time this method is called.
		}

		if (!this->pPlayer->st.HP)
			CueEvents.Add(CID_MonsterKilledPlayer, this->pCombat->pMonster);
	}
}

//*****************************************************************************
void CCurrentGame::ProcessMonsterDefeat(
//When a monster is defeated in combat or otherwise (e.g. a deadly NPC sword hit
//performs an instant kill), process the defeat here.
//
//Params:
	CCueEvents& CueEvents,
	CMonster* pDefeatedMonster,     //defeated monster
	const UINT wSX, const UINT wSY, //location where combat took place
	const UINT wSwordMovement)      //direction of sword hit
{
	if (pDefeatedMonster->OnStabbed(CueEvents, wSX, wSY))
	{
		//Monster was killed.
		if (CueEvents.HasOccurredWith(CID_MonsterDiedFromStab, pDefeatedMonster))
		{
			//Store info about stab effect.
			pDefeatedMonster->SetKillInfo(wSwordMovement);

			this->pRoom->KillMonster(pDefeatedMonster, CueEvents);

			switch (pDefeatedMonster->wType)
			{
				case M_MUDMOTHER:
				case M_TARMOTHER:
				case M_GELMOTHER:
					//Tarstuff tile should be removed at end of combat.
					if (bIsTar(this->pRoom->GetTSquare(wSX, wSY))
							) //always remove this tar tile when defeated //&& this->pRoom->StabTar(wSX, wSY, CueEvents, false))
						this->simulSwordHits.push_back(CMoveCoord(wSX, wSY, wSwordMovement));
				break;
			}
		}
		else if (CueEvents.HasOccurredWith(CID_SnakeDiedFromTruncation, pDefeatedMonster))
		{
			ASSERT(bIsSerpent(pDefeatedMonster->wType));
			ASSERT(pDefeatedMonster->bAlive);
			this->pRoom->KillMonster(pDefeatedMonster, CueEvents);
		}
	} else {
		//If an NPC is defeated but not killed, execute any script commands
		//dealing with processing the NPC's defeat immediately (on this turn, not the next).
		if (pDefeatedMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pDefeatedMonster);
			this->bExecuteNoMoveCommands = true;
			pCharacter->ProcessAfterDefeat(CueEvents);
			this->bExecuteNoMoveCommands = false;
			CueEvents.Add(CID_NPC_Defeated, pDefeatedMonster);
		}
	}

	//Each time a monster is fought, briar roots expand.
	this->pRoom->ExpandBriars(CueEvents);
}

//*****************************************************************************
int CCurrentGame::getItemAmount(const UINT item) const
//Returns: the usage amount of this item on the current level with current game state
//
//Note that these values may either be positive or negative.
//The game engine should robustly support any possible value.
{
	ASSERT(this->pLevel);
	const UINT levelVal = this->pLevel->getItemAmount(item); //get level's item value

	//Apply general item multiplier and item-specific multiplier.
	ASSERT(this->pPlayer);
	float fMult = GetTotalStatModifier(ScriptVars::ItemAll); //may be negative
	switch (item)
	{
		case T_HEALTH_BIG: case T_HEALTH_MED: case T_HEALTH_SM:
			fMult *= GetTotalStatModifier(ScriptVars::ItemHP);
		break;
		case T_ATK_UP:
			fMult *= GetTotalStatModifier(ScriptVars::ItemATK);
		break;
		case T_DEF_UP:
			fMult *= GetTotalStatModifier(ScriptVars::ItemDEF);
		break;
		case T_DOOR_MONEY: case T_DOOR_MONEYO:
			fMult *= GetTotalStatModifier(ScriptVars::ItemGR);
		break;
		default: break;
	}
	return intBounds(levelVal * fMult);
}

//*****************************************************************************
bool CCurrentGame::equipmentBlocksGaze(const UINT type) const
//Returns: whether the specified inventory item can block the aumtlich gaze
{
	if (type == ScriptFlag::Weapon && this->pPlayer->st.sword == ReallyBigSword)
		return true;

	CCharacter* pCharacter = getCustomEquipment(type);
	if (pCharacter && pCharacter->HasRayBlocking())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::CanPlayerCutBriars() const
//Returns: whether player can cut briars
{
	if (this->pPlayer->st.sword == BriarSword)
		return true;

	CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Weapon);
	if (pCharacter && pCharacter->CanCutBriar())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Armor);
	if (pCharacter && pCharacter->CanCutBriar())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter && pCharacter->CanCutBriar())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::CustomNPCExists(const UINT characterID) const
//Returns: whether a custom NPC with the specified ID is defined for the current hold
{
	if (!this->pHold)
		return false;
	
	return this->pHold->GetCharacter(characterID) != NULL;
}

//*****************************************************************************
int CCurrentGame::getPlayerATK() const
//Returns: player's current ATK power
{
	const PlayerStats& st = this->pPlayer->st;
	int atk = st.ATK;

	if (!IsPlayerSwordDisabled())
		addWithClamp(atk, getWeaponPower(st.sword));

	//ATK modifiers for other equipment is also taken into account.
	if (bIsCustomEquipment(st.shield) && !IsPlayerShieldDisabled())
	{
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Armor);
		if (pCharacter)
			addWithClamp(atk, (int)pCharacter->getATK());
	}
	if (bIsCustomEquipment(st.accessory) && !IsPlayerAccessoryDisabled())
	{
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Accessory);
		if (pCharacter)
			addWithClamp(atk, (int)pCharacter->getATK());
	}

	return atk;
}

//*****************************************************************************
int CCurrentGame::getPlayerDEF() const
//Returns: player's DEF power
{
	const PlayerStats& st = this->pPlayer->st;
	int def = st.DEF;

	if (!IsPlayerShieldDisabled())
		addWithClamp(def, getShieldPower(st.shield));

	//DEF modifiers for other equipment is also taken into account.
	if (bIsCustomEquipment(st.sword) && !IsPlayerSwordDisabled())
	{
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Weapon);
		if (pCharacter)
			addWithClamp(def, (int)pCharacter->getDEF());
	}
	if (bIsCustomEquipment(st.accessory) && !IsPlayerAccessoryDisabled())
	{
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Accessory);
		if (pCharacter)
			addWithClamp(def, (int)pCharacter->getDEF());
	}

	return def;
}

//*****************************************************************************
int CCurrentGame::getPredefinedShieldPower(const UINT type)
//Returns: the power of this (pre-defined) shield
{
	switch (type)
	{
		default:
		case ArmorSlot:
		case NoShield: return 0;

		case WoodenShield: return 10;  //can use on oremites
		case BronzeShield: return 30;
		case SteelShield: return 70;
		case KiteShield: return 120;
		case OremiteShield: return 220;  //can use on oremites
	}
}

//*****************************************************************************
int CCurrentGame::getPredefinedWeaponPower(const UINT type)
//Returns: the power of this (pre-defined) sword
{
	switch (type)
	{
		default:
			//no break
		case WeaponSlot:
		case NoSword: return 0;

		case WoodenBlade: return 10;  //can use on oremites
		case ShortSword: return 30;
		case GoblinSword: return 30; //x2ATK: goblin + goblinking
		case LongSword: return 70;
		case HookSword: return 120;
		case ReallyBigSword: return 220;
		case LuckySword: return 10;    //x2 GOLD
		case SerpentSword: return 120; //x2ATK: serpents
		case BriarSword: return 10;    //breaks briars
	}
}

//*****************************************************************************
int CCurrentGame::getShieldPower(const UINT type) const
//Returns: the power of this armor type
{
	if (bIsCustomEquipment(type))
	{
		//Get the current DEF power of the custom weapon script.
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Armor);
		if (pCharacter)
			return (int)pCharacter->getDEF();
	}
	return getPredefinedShieldPower(type);
}

//*****************************************************************************
int CCurrentGame::getWeaponPower(const UINT type) const
//Returns: the power of this weapon type
{
	if (bIsCustomEquipment(type))
	{
		//Get the current ATK power of the custom weapon script.
		CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Weapon);
		if (pCharacter)
			return (int)pCharacter->getATK();
	}
	return getPredefinedWeaponPower(type);
}

//*****************************************************************************
bool CCurrentGame::IsFighting(CMonster* pMonster) const
//Returns: whether the indicated entity is engaged in combat
{
	return this->pCombat && this->pCombat->IsFighting(pMonster);
}

//*****************************************************************************
bool CCurrentGame::IsLuckyGRItem(const UINT type) const
//Returns: whether the player's specified inventory item is lucky (gives x2 gold)
{
	if (type == ScriptFlag::Weapon &&
			this->pPlayer->st.sword == LuckySword)
		return true;
	if (type == ScriptFlag::Accessory &&
			this->pPlayer->st.accessory == LuckyGold)
		return true;

	CCharacter* pCharacter = getCustomEquipment(type);
	if (pCharacter && pCharacter->IsLuckyGR())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::IsLuckyXPItem(const UINT type) const
//Returns: whether the player's specified inventory item gives x2 XP
{
	if (type == ScriptFlag::Accessory &&
			this->pPlayer->st.accessory == XPDoubler)
		return true;

	CCharacter* pCharacter = getCustomEquipment(type);
	if (pCharacter && pCharacter->IsLuckyXP())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::DoesPlayerAttackFirst() const
//Returns: whether the player has an ability to attack first in combat.
{
	if (this->pPlayer->IsHasted() || this->pPlayer->IsInvisible())
		return true;

	CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Weapon);
	if (pCharacter && pCharacter->CanAttackFirst())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Armor);
	if (pCharacter && pCharacter->CanAttackFirst())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter && pCharacter->CanAttackFirst())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::DoesPlayerAttackLast() const
//Returns: whether the player has an ability to always attack last in combat.
{
	if (this->pPlayer->IsHasted() || this->pPlayer->IsInvisible())
		return false; //these attributes override a last attack behavior

	CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Weapon);
	if (pCharacter && pCharacter->CanAttackLast())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Armor);
	if (pCharacter && pCharacter->CanAttackLast())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter && pCharacter->CanAttackLast())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::DoesPlayerBackstab() const
//Returns: whether the player has an ability to perform a strong backstab attack
{
	CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Weapon);
	if (pCharacter && pCharacter->TurnToFacePlayerWhenFighting())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Armor);
	if (pCharacter && pCharacter->TurnToFacePlayerWhenFighting())
		return true;
	pCharacter = getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter && pCharacter->TurnToFacePlayerWhenFighting())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::DoesPlayerItemHaveNoEnemyDefense(const UINT type) const
//Returns: whether the player has a weapon or accessory that nullifies the enemy's DEF,
//         or armor that protects against nullified DEF
{
	//No predefined weapons have this attribute.

	CCharacter* pCharacter = getCustomEquipment(type);
	if (pCharacter && pCharacter->HasNoEnemyDefense())
		return true;

	return false;
}

//*****************************************************************************
bool CCurrentGame::DoesTileDisableMetal(const UINT wX, const UINT wY) const
//Returns: whether the tile being considered will prevent using metal equipment
{
	//Oremites on tile disable metal
	const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
	return wOTile == T_GOO;
}

//*****************************************************************************
bool CCurrentGame::IsPlayerShieldDisabled() const
//Returns: true if the player's shield is not usable at present, else false
{
	CSwordsman& player = *this->pPlayer;
	if (!player.IsInRoom())
		return false;
	return player.bShieldOff ||
			(DoesTileDisableMetal(player.wX, player.wY) && IsShieldMetal(player.st.shield));
}

//*****************************************************************************
bool CCurrentGame::IsPlayerSwordDisabled() const
//Returns: true if the player's sword is not usable at present, else false
{
	return this->pPlayer->IsSwordDisabled();
}

//*****************************************************************************
bool CCurrentGame::IsPlayerAccessoryDisabled() const
//Returns: true if the player's accessory is not usable at present, else false
{
	CSwordsman& player = *this->pPlayer;
	if (!player.IsInRoom())
		return false;

	if (player.bAccessoryOff)
		return true;

	//Metal?
	if (!DoesTileDisableMetal(player.wX, player.wY))
		return false;
	CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter)
		return pCharacter->IsMetal();

	return false;
}

//*****************************************************************************
bool CCurrentGame::IsSwordMetal(const UINT type) const
//Returns: whether this sword type is metal
{
	switch (type)
	{
		case NoSword:
		case WoodenBlade:
			return false;
		default:
		{
			CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Weapon);
			if (pCharacter)
				return pCharacter->IsMetal();
		}
		return true;
	}
}

//*****************************************************************************
bool CCurrentGame::IsShieldMetal(const UINT type) const
//Returns: whether this shield type is metal
{
	switch (type)
	{
		case NoShield:
		case WoodenShield:
		case OremiteShield:
			return false;
		default:
		{
			CCharacter* pCharacter = getCustomEquipment(ScriptFlag::Armor);
			if (pCharacter)
				return pCharacter->IsMetal();
		}
		return true;
	}
}

//*****************************************************************************
bool CCurrentGame::IsSwordStrongAgainst(CMonster* pMonster) const
//Returns: whether the player's sword is strong against this monster
{
	const UINT sword = this->pPlayer->st.sword;
	switch (sword)
	{
		case GoblinSword:
			return pMonster->HasGoblinWeakness();
		case SerpentSword:
			return pMonster->HasSerpentWeakness();
		default:
		{
			if (IsEquipmentStrongAgainst(pMonster, ScriptFlag::Weapon))
				return true;
		}
		return false;
	}
}

//*****************************************************************************
bool CCurrentGame::IsEquipmentStrongAgainst(CMonster* pMonster, const UINT type) const
//Returns: whether the player has an item that is strong against this monster
{
	CCharacter* pCharacter = getCustomEquipment(type);
	if (pCharacter)
	{
		if (pMonster->HasGoblinWeakness() && pCharacter->HasGoblinWeakness())
			return true;
		if (pMonster->HasSerpentWeakness() && pCharacter->HasSerpentWeakness())
			return true;
	}

	return false;
}

//*****************************************************************************
void CCurrentGame::ProcessSwordHit(
//Processes results of a sword (mimic or swordsman) entering a square.
//
//Params:
	const UINT wSX, const UINT wSY,        //(in)   Square sword is in.
	CCueEvents &CueEvents,     //(out)  List of events that can be handled by caller.
								//    These are things that the UI wouldn't necessarily
								//    be aware of by looking at the modified game
								//    data on return.
	CMonster *pDouble)      //(in)   If NULL (default) this call is checking the 
								//    player's sword.  Otherwise, this will be a 
								//    pointer to a double and the call will be checking
								//    that double's sword.
{
	if (!this->pRoom->IsValidColRow(wSX, wSY))
		return;

	CPlayerDouble *pPlayerDouble = dynamic_cast<CPlayerDouble*>(pDouble);
	const UINT wSwordMovement = pPlayerDouble ? pPlayerDouble->wSwordMovement :
			GetSwordMovement();

	//Did sword hit a monster?
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wSX, wSY);
	if (pMonster && //Yes.
			//don't allow critical character kill on room entrance
			(this->wTurnNo || !this->pRoom->IsMonsterOfTypeAt(M_HALPH, wSX, wSY, true)))
	{
		if (pMonster->IsPiece())
		{
			CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
			pMonster = pPiece->pMonster;
		}

		if (!pDouble)
		{
			//Player hits monster first.
			bool dontInitiate = false;

			if (InCombat())
			{
				ASSERT(this->pCombat);
				if (this->pCombat->pMonster == pMonster)
				{
					//Player has probably started a room with their sword on a monster
					//and bumped into it on their first turn.
					//When this happens, the bump will initiate combat.
					//However, because the sword is also on the monster,
					//the sword strike will take precedence.
					delete this->pCombat;
					this->pCombat = NULL;
				} else {
					// If that's not the case we can assume, that the player's sword is lying
					// on a monster that they can't damage and can't be killed by, and the player 
					// is bumping into an another monster
					this->pCombat->QueueMonster(pMonster, true, 
							this->pPlayer->wX, this->pPlayer->wY, wSX, wSY);
					dontInitiate = true;
				}
			}

			if (!dontInitiate)
				InitiateCombat(CueEvents, pMonster, true,
						this->pPlayer->wX, this->pPlayer->wY, wSX, wSY);
		} else {
			switch (pDouble->wType)
			{
				case M_MIMIC:
					//When a mimic stabs a monster, then the player must fight it.
					//Player gets first hit, just as if the player had stabbed the monster.
					InitiateCombat(CueEvents, pMonster, true,
							pDouble->wX, pDouble->wY, wSX, wSY);
				break;
				case M_CHARACTER:
				{
					//If a "deadly" character strikes this monster, consider it
					//an instant kill.
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pDouble);
					if (!pCharacter->IsSwordSafeToPlayer())
						ProcessMonsterDefeat(CueEvents, pMonster, wSX, wSY, wSwordMovement);
				}
				break;
				default:
					//otherwise, monster's sword doesn't hurt an adjacent monster at all
				break;
			}
		}
	}

	//
	//Check for things in T-square for sword to hit.
	//
	UINT wTileNo = this->pRoom->GetTSquare(wSX, wSY);
	switch (wTileNo)
	{
		case T_ORB: 
			this->pRoom->ActivateOrb(wSX, wSY, CueEvents, pDouble ? OAT_Monster : OAT_Player);
		break;

		case T_TAR:	case T_MUD: case T_GEL:
			if (!pMonster &&  //if tar stab isn't being handled by a fight with a monster above
					this->pRoom->StabTar(wSX, wSY, CueEvents, false)) //don't remove tar yet!
			{
				//Stab hits vulnerable tarstuff.

				//If a mother is part of this tarstuff's connected component,
				//then it is considered a part of the mother,
				//and the mother must be defeated to break this piece of tarstuff.
				//The mother's HP is restored after breaking the tarstuff tile.
				CMonster *pMother = NULL;

				//Currently applies to player or mimics only.
				if (!pDouble || pDouble->wType == M_MIMIC)
					pMother = this->pRoom->GetMotherConnectedToTarTile(wSX, wSY);
				if (pMother && !IsFighting(pMother))
				{
					InitiateCombat(CueEvents, pMother, true, 0, 0, wSX, wSY, true);
					this->possibleTarStabs.push_back(TarstuffStab(CMoveCoord(wSX,wSY,wSwordMovement), pMother));
				} else {
					//Mark for removal at end of turn
					this->simulSwordHits.push_back(CMoveCoord(wSX,wSY,wSwordMovement));
				}
			}
		break;

		case T_BOMB:
		   //Explode bomb immediately (could damage player).
			if (this->wTurnNo)   //don't explode bomb and damage/kill player on room entrance
			{
				CCoordStack bomb(wSX, wSY);
				this->pRoom->BombExplode(CueEvents, bomb);
			}
		break;

		//Cut briar tiles if entity has briar cutting attribute.
		case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			if ((pDouble && pDouble->CanCutBriar()) || (!pDouble && CanPlayerCutBriars()))
			{
				this->pRoom->Plot(wSX, wSY, T_EMPTY);
				CueEvents.Add(CID_CutBriar, new CMoveCoord(wSX, wSY, wSwordMovement), true);
			}
		break;
		case T_BRIAR_SOURCE:
			if ((pDouble && pDouble->CanCutBriar()) || (!pDouble && CanPlayerCutBriars()))
			{
				this->pRoom->Plot(wSX, wSY, T_EMPTY);
				this->pRoom->briars.removeSource(wSX, wSY);
				CueEvents.Add(CID_CutBriar, new CMoveCoord(wSX, wSY, wSwordMovement), true);
			}
		break;

		case T_MIRROR:
		{
			//Swords smash mirror blocks when striking head on, otherwise they push them.
			UINT wSrcX, wSrcY;   //where sword wielder is standing
			if (pPlayerDouble)
			{
				wSrcX = pPlayerDouble->wX;
				wSrcY = pPlayerDouble->wY;
			} else {
				wSrcX = this->pPlayer->wX;
				wSrcY = this->pPlayer->wY;
			}
			if (wSrcX + nGetOX(wSwordMovement) == wSX && wSrcY + nGetOY(wSwordMovement) == wSY)
			{
				//Head on strike shatters mirror.
				this->pRoom->Plot(wSX, wSY, T_EMPTY);
				CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wSX, wSY, wSwordMovement), true);
			} else {
				//Slide mirror, if possible.
				const UINT wDestX = wSX + nGetOX(wSwordMovement),
						wDestY = wSY + nGetOY(wSwordMovement);
				if (this->pRoom->CanPushTo(wSX, wSY, wDestX,	wDestY))
					this->pRoom->PushObject(wSX, wSY, wDestX,	wDestY, CueEvents);
			}
		}
		break;
	}

	//
	//Check for things in O-square for sword to hit.
	//

	//Did sword hit a crumbly wall?
	wTileNo = this->pRoom->GetOSquare(wSX, wSY);
	switch (wTileNo)
	{
		case T_WALL_B:
		case T_WALL_H:
		   this->pRoom->DestroyCrumblyWall(wSX, wSY, CueEvents, wSwordMovement);
		break;
		default: break;
	}

	//
	//Mimic-only checks.
	//

	if (pDouble)
	{
		//Hit the player?
		if (IsPlayerAt(wSX, wSY)
//				&& pPlayer->IsStabbable()
				&& this->wTurnNo) //don't kill player on room entrance
		{
			//If the character's sword has been marked as safe to the player,
			//then sword stabs won't cause damage.
			if (pDouble->wType == M_CHARACTER)
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pDouble);
				if (pCharacter->IsSwordSafeToPlayer())
					return;
			}

			//Stabs the player.
			//This doesn't initiate a full combat sequence,
			//like a sword hit from the player does.
			CCombat combat(this, pDouble, false);
			combat.MonsterAttacksPlayerOnce(CueEvents);
			CueEvents.Add(CID_PlayerStabbed);
		}
	}
}

//*****************************************************************************
void CCurrentGame::RestartRoom(
//Restarts the current room.
//
//Params:
	CCueEvents &CueEvents)     //(out)  Cue events generated by swordsman's
								//    first step into the room.
{
/*
	//If in the middle of recording a demo, stop recording.
	UINT wBeginTurn = 0;
	const bool bRecordingDemo = IsDemoRecording();
	if (bRecordingDemo)
	{
		this->bIsDemoRecording = false;
		wBeginTurn = this->DemoRecInfo.wBeginTurnNo; //preserve when demo recording began
	}
*/

/*
	//If this is the first time this room has been entered, make sure it will
	//remain flagged this way after reloading the room below.
	SavePrep();
*/

	//Return room to the state it had on entry.
	ASSERT(this->pRoom);
	this->pRoom->Reload();
	CDbSavedGame::ReloadMonsterList();
	RetrieveExploredRoomData(*this->pRoom);

	//Move the player back to the beginning of the room.
	CueEvents.Clear();
	SetPlayerToRoomStart();
	SetMembersAfterRoomLoad(CueEvents);
	ProcessCommand_EndOfTurnEventHandling(CueEvents);
/*
	//If player was recording a demo from the beginning of the room,
	//then resume recording from this point.
	if (bRecordingDemo && !wBeginTurn)
		this->bIsDemoRecording = true;
*/
}

//*****************************************************************************
/*
void CCurrentGame::PostSave(const bool bConqueredOnEntrance, const bool bExploredOnEntrance)
//Call this following a call to SavePrep and recording a save game record
//to restore game flags the way they were before saving.
{
	if (this->bIsNewRoom)
	{
		if (bExploredOnEntrance)
			SetCurrentRoomExplored();
		if (bConqueredOnEntrance)
			SetCurrentRoomConquered();
	}
}

//-****************************************************************************
bool CCurrentGame::SavePrep(bool& bExploredOnEntrance) //(out)
//If this is the first time the current room has been entered,
//unflag explored+conquered before saving to ensure the room will
//be re-flagged the way it is now when loading the saved game.
//
//Call PostSave after saving to undo these temporary changes.
//
//Returns: whether new room was conquered on entrance
{
	bool bConqueredOnEntrance = false;
	bExploredOnEntrance = false;
	if (this->bIsNewRoom)
	{
		if (IsCurrentRoomExplored())
		{
			bExploredOnEntrance = true;
			this->ExploredRooms -= this->pRoom->dwRoomID;
		}
		bConqueredOnEntrance = IsCurrentRoomConquered();
		if (bConqueredOnEntrance)
			this->ConqueredRooms -= this->pRoom->dwRoomID;
	}
	return bConqueredOnEntrance;
}
*/

//*****************************************************************************
void CCurrentGame::QuickSave()
//Save the current player's game to the quicksave slot.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Must store what state game was in on room entrance, so when moves are
	//replayed, we'll end up at the current state once more.
	CDbPackedVars _stats = this->stats;
	this->stats = this->statsAtRoomStart;

	WSTRING locText;
	locText += this->pLevel->NameText;
	locText += wszColon;
	locText += wszSpace;
	this->pRoom->GetLevelPositionDescription(locText, true);

	//Set saved game ID to current player's quicksave slot for this hold.
	UINT quicksaveID = g_pTheDB->SavedGames.FindByContinue(ST_Quicksave);
	if (!quicksaveID)
		quicksaveID = g_pTheDB->SavedGames.SaveNewContinue(g_pTheDB->GetPlayerID(), ST_Quicksave);
	this->wVersionNo = VERSION_NUMBER;
	this->dwSavedGameID = quicksaveID;
	this->eType = ST_Quicksave;
	this->stats.SetVar(szSavename, locText.c_str());
	this->bIsHidden = true;
	Update();

	//Update player's time stamp.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	pPlayer->Update();
	delete pPlayer;

	this->stats = _stats; //revert
}

//*****************************************************************************
void CCurrentGame::SaveToContinue()
//Save the current player's game to the continue slot for this hold.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	/*
	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	*/

	//Must store what state game was in on room entrance, so when moves are
	//replayed, we'll end up at the current state once more.
	CDbPackedVars _stats = this->stats;
	this->stats = this->statsAtRoomStart;

	WSTRING locText;
	locText += this->pLevel->NameText;
	locText += wszColon;
	locText += wszSpace;
	this->pRoom->GetLevelPositionDescription(locText, true);

	//Set saved game ID to current player's continue slot for this hold.
	UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
	if (!dwContinueID)
		dwContinueID = g_pTheDB->SavedGames.SaveNewContinue(g_pTheDB->GetPlayerID());
	this->wVersionNo = VERSION_NUMBER;
	this->dwSavedGameID = dwContinueID;
	this->eType = ST_Continue;
	this->stats.SetVar(szSavename, locText.c_str());

	this->bIsHidden = true;
	Update();

	//Update player's time stamp.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	pPlayer->Update();
	delete pPlayer;

	this->stats = _stats; //revert

	/*
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
	*/
}

//*****************************************************************************
void CCurrentGame::SaveToEndHold()
//Save the current player's game to the end hold slot for this hold.
//It is overwritten each time the hold is ended.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Mark completed rooms.
	AddRoomsToPlayerTally();

	//Set saved game ID to current player's end hold slot.
	const UINT dwEndHoldID = g_pTheDB->SavedGames.FindByEndHold(this->pHold->dwHoldID);
	this->dwSavedGameID = dwEndHoldID;
	this->wVersionNo = VERSION_NUMBER;
	this->eType = ST_EndHold;
	PackData(this->stats);
	this->stats.SetVar(szSavename, wszEmpty);
	this->bIsHidden = true;
	Update();
}

//*****************************************************************************
/*
void CCurrentGame::SaveToLevelBegin()
//Saves the current game to the level-begin slot for this level.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Swordsman should be at beginning of level entry room.
	ASSERT(this->wTurnNo == 0);
	ASSERT(this->pPlayer->wX == this->wStartRoomX);
	ASSERT(this->pPlayer->wY == this->wStartRoomY);
	ASSERT(this->pPlayer->wO == this->wStartRoomO);
	ASSERT(this->pRoom->dwRoomID == this->dwRoomID);
	ASSERT(this->pPlayer->wIdentity == this->wStartRoomAppearance);
	ASSERT(this->pPlayer->bSwordOff == this->bStartRoomSwordOff);
	...new equipment disabled stats

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	this->eType = ST_LevelBegin;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this level?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByLevelBegin(
			this->pRoom->dwLevelID);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//-****************************************************************************
void CCurrentGame::SaveToRoomBegin()
//Saves the current game to the begin-room slot for this room.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Swordsman should be at beginning of room.
	ASSERT(this->wTurnNo == 0);
	ASSERT(this->pPlayer->wX == this->wStartRoomX);
	ASSERT(this->pPlayer->wY == this->wStartRoomY);
	ASSERT(this->pPlayer->wO == this->wStartRoomO);
	ASSERT(this->pRoom->dwRoomID == this->dwRoomID);
	ASSERT(this->pPlayer->wIdentity == this->wStartRoomAppearance);
	ASSERT(this->pPlayer->bSwordOff == this->bStartRoomSwordOff);
	...new equipment disabled stats

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	this->eType = ST_RoomBegin;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this room?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByRoomBegin(
			this->pRoom->dwRoomID);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//-****************************************************************************
void CCurrentGame::SetComputationTimePerSnapshot(const UINT dwTime)
//Set the amount of move calculation time to elapse between game state snapshots.
{
	//Practical lower bound on time between snapshots.
	static const UINT dwMinTime = 100; //ms

	this->dwComputationTimePerSnapshot = dwTime;
	if (this->dwComputationTimePerSnapshot < dwMinTime)
		this-> dwComputationTimePerSnapshot = dwMinTime;
}
*/

//*****************************************************************************
void CCurrentGame::TeleportPlayer(
	//Attempt to teleport player to a new square and init all changes
	//
	//Params:
	const UINT wSetX, const UINT wSetY,  //(in)   Coords of new square.
	CCueEvents& CueEvents)
{
	if (!this->pRoom->IsValidColRow(wSetX, wSetY))
	{
		return;
	}

	if (this->pRoom->DoesSquareContainTeleportationObstacle(wSetX, wSetY, this->pPlayer->wIdentity)) {
		return;
	}

	// Teleporting to the tile the player is already standing at should do nothing
	if (IsPlayerAt(wSetX, wSetY))
	{
		this->pPlayer->wPrevX = wSetX;
		this->pPlayer->wPrevY = wSetY;
		return;
	}

	const UINT wTTileNo = this->pRoom->GetTSquare(this->pPlayer->wX, this->pPlayer->wY);
	const bool bWasOnSameScroll = wTTileNo == T_SCROLL;

	this->pPlayer->wSwordMovement = NO_ORIENTATION;

	SetPlayer(wSetX, wSetY);

	this->pPlayer->SetSwordSheathed();

	ProcessPlayerMoveInteraction(0, 0, CueEvents, bWasOnSameScroll, true, true);

	this->pRoom->CheckForFallingAt(wSetX, wSetY, CueEvents);

	this->pPlayer->bHasTeleported = true;
}

//*****************************************************************************
bool CCurrentGame::SetPlayer(
//Move player to new square.
//
//Returns: whether the player moved to a new square
//
//Params:
	const UINT wSetX, const UINT wSetY) //(in)   Coords of new square.
{
	ASSERT(this->pRoom->IsValidColRow(wSetX, wSetY));

	const bool bMoved = this->pPlayer->Move(wSetX,wSetY);
/*
	//Reset PathMaps' target.
	if (bMoved && this->pPlayer->IsTarget())
		this->pRoom->SetPathMapsTarget(this->pPlayer->wX, this->pPlayer->wY);
*/
	return bMoved;
}

//*****************************************************************************
void CCurrentGame::SetPlayerRole(const UINT wType)
//Sets the player's appearance to indicated monster/character type.
{
	this->pPlayer->wAppearance = this->pPlayer->wIdentity = wType;
	if (wType >= CUSTOM_CHARACTER_FIRST && wType != M_NONE)
	{
		//When logical role is a custom value, use its designated appearance
		//for behavioral semantics.
		ASSERT(this->pHold);
		HoldCharacter *pChar = this->pHold->GetCharacter(wType);
		if (pChar)
			this->pPlayer->wAppearance = pChar->wType;
		else
			//This is a dangling custom character reference.
			//Ignore it and use the default role.
			this->pPlayer->wAppearance = this->pPlayer->wIdentity = defaultPlayerType();
	}

/*
	//When player's role changes, brain pathmap needs to be updated.
	if (this->swordsman.IsTarget() && this->pRoom &&
			this->pRoom->pCurrentGame) //wait until the room and game have been connected
		this->pRoom->SetPathMapsTarget(this->swordsman.wX, this->swordsman.wY);
*/
}

//*****************************************************************************
bool CCurrentGame::SetPlayerSwordSheathed()
//Sets and returns whether player's sword is sheathed.
{
	CSwordsman& player = *this->pPlayer;
	return player.bSwordSheathed = DoesTileDisableMetal(player.wX, player.wY) && IsSwordMetal(player.st.sword);
}

//*****************************************************************************
void CCurrentGame::SetTurn(
//Sets current game to a specific turn, without removing commands.
//
//Params: 
	UINT wTurnNo,        //(in)   Turn to which game will be set.
	CCueEvents &CueEvents)  //(out)  Cue events generated by the last command.
{ 
	ASSERT(this->pRoom);
	if (wTurnNo > this->Commands.Count())	//bounds check
		wTurnNo = this->Commands.Count();

/*
	//If this is the first time a new room has been entered, make sure it will
	//also be marked this way when reloading this saved game.
	if (this->bIsNewRoom)
		this->ExploredRooms -= this->pRoom->dwRoomID;

	//If game state snapshots have been maintained during play,
	//find the latest one before the turn number being replayed to.
	//(We don't want to restore to the exact turn a snapshot was taken,
	//because without replaying the final turn, we won't have any cue events
	//to display the events of this turn in the front end.)
	CCurrentGame *pSnapshot;
	for (pSnapshot = this->pSnapshotGame; pSnapshot != NULL;
			pSnapshot = pSnapshot->pSnapshotGame)
	{
		if (pSnapshot->wTurnNo < wTurnNo)
			break; //this is the closest one before the turn to replay to
	}
	if (pSnapshot)
	{
		//Delete snapshots taken after this snapshot.
		CCurrentGame *pLaterSnapshot = this->pSnapshotGame;
		while (pLaterSnapshot != pSnapshot)
		{
			CCurrentGame *pDelete = pLaterSnapshot;
			pLaterSnapshot = pLaterSnapshot->pSnapshotGame;
			pDelete->pSnapshotGame = NULL; //don't destroy the earlier snapshots
			delete pDelete;
		}

		//Preserve member vars that shouldn't change on game state reversion.
		CDbCommands commands(this->Commands);
/-*
		const UINT d_ = this->dwLevelDeaths, k_ = this->dwLevelKills,
				m_ = this->dwLevelMoves, t_ = this->dwLevelTime,
				st_ = this->dwStartTime;
		const bool rl_ = this->bRoomExitLocked;
		const UINT dwAutoSaveOptions_ = this->dwAutoSaveOptions;
//

		//Restore game to state of selected snapshot.
		CueEvents.Clear();
		SetMembers(*pSnapshot);

		//Hook in this and earlier snapshots.
		this->pSnapshotGame = pSnapshot;

		//Restore command list.
		this->Commands = commands; //Commands may or may not be truncated by caller.

		//Replay moves from time of snapshot to target turn number.
		if (this->wTurnNo < wTurnNo)
			VERIFY(PlayCommands(wTurnNo, CueEvents));

/-*
		//Restore stats that shouldn't change.
		this->dwLevelDeaths = d_;
		this->dwLevelKills = k_;
		this->dwLevelMoves = m_;
		this->dwLevelTime = t_;
		this->dwStartTime = st_;
		this->bRoomExitLocked = rl_;
		this->dwAutoSaveOptions = dwAutoSaveOptions_;
//

		return;
	}
*/

	//Freeze commands as a precaution--nothing below should change commands.
	FreezeCommands();

	//Return room to the state it had on entry.
	this->pRoom->Reload();
	CDbSavedGame::ReloadMonsterList();
	RetrieveExploredRoomData(*this->pRoom);

	//Move the player back to the beginning of the room.
	CueEvents.Clear();
	SetPlayerToRoomStart();
	SetMembersAfterRoomLoad(CueEvents, false);
	ProcessCommand_EndOfTurnEventHandling(CueEvents);

	UnfreezeCommands();

	//Play the commands back.
	if (wTurnNo)
		PlayCommands(wTurnNo, CueEvents);
} 

//*****************************************************************************
void CCurrentGame::UndoCommand(
//Undoes one command.
//
//Params:
	CCueEvents &CueEvents)  //(out)  Cue events generated by the new last command.
{
	if (this->Commands.Empty()) return; //No commands to undo.

	UndoCommands(1, CueEvents);
}

//*****************************************************************************
void CCurrentGame::UndoCommands( 
//Undos one or more commands by restarting current room and replaying recorded moves to 
//reach the current turn minus a specified number of undone commands.
// 
//Params: 
	const UINT wUndoCount,  //(in)   Number of commands to undo.
	CCueEvents &CueEvents)  //(out)  Cue events generated by the new last command.
{ 
	ASSERT(this->pRoom);
	ASSERT(wUndoCount > 0 && wUndoCount <= this->Commands.Count());
	UINT wPlayCount = this->Commands.Count() - wUndoCount;

/*
	//If an undo is performed while a demo is being recorded,
	//keep recording the demo and remove the previous command.
	const bool bDemoRecording = IsDemoRecording();
	UINT wBeginTurnNo = 0;
	if (bDemoRecording)
	{
		wBeginTurnNo = this->DemoRecInfo.wBeginTurnNo;	//room reload clears this value
		this->bIsDemoRecording = false;
	}
*/

/*
	//If this is the first time this room has been entered, make sure it will
	//remain flagged this way after reloading the room below.
	SavePrep();
*/

	//Save values that get reset by undoing.
	const UINT time = this->pPlayer->st.totalTime;

	//Play the commands back, minus undo count.
	SetTurn(wPlayCount, CueEvents);
	this->Commands.Truncate(wPlayCount);

	//Restore saved values.
	this->pPlayer->st.totalTime = time;

/*
	//If player was recording a demo, starting from before this turn,
	//then resume recording from this point.
	if (bDemoRecording && wBeginTurnNo <= wPlayCount)
	{
		this->bIsDemoRecording = true;
		this->DemoRecInfo.wBeginTurnNo = wBeginTurnNo;
	}
*/
} 

//*****************************************************************************
void CCurrentGame::UnfreezeCommands()
//Allow modification of command list after a call to FreezeCommands().
{
	//Commands should not have been frozen while recording.  Recording requires 
	//commands to be added for each call to ProcessCommand().  It is possible to have 
	//some carefully thought-out mixture of the two states, in which case this assertion 
	//can be changed.
//	ASSERT(!this->bIsDemoRecording);

	this->Commands.Unfreeze();
}

//*****************************************************************************
void CCurrentGame::UpdatePrevCoords()
//Update previous coords for player and monsters to current positions.
{
	this->pPlayer->wPrevX = this->pPlayer->wX;
	this->pPlayer->wPrevY = this->pPlayer->wY;
	this->pPlayer->wPrevO = this->pPlayer->wO;

	CMonster *pMonster = this->pRoom->pFirstMonster;
	while (pMonster)
	{
		pMonster->wPrevX = pMonster->wX;
		pMonster->wPrevY = pMonster->wY;
		pMonster->wPrevO = pMonster->wO;
		pMonster = pMonster->pNext;
	}

	UpdatePrevPlatformCoords();
}

//*****************************************************************************
void CCurrentGame::UpdatePrevPlatformCoords()
{
	for (vector<CPlatform*>::iterator platformIter = this->pRoom->platforms.begin();
			platformIter != this->pRoom->platforms.end(); ++platformIter)
	{
		CPlatform& platform = *(*platformIter);
		platform.xDelta = platform.yDelta = 0;
	}
}

//*****************************************************************************
void CCurrentGame::UpdateTime(const UINT dwTime)  //[default=0]
//Keeps track of real time played.  Increments total each time called.
//Accurate to the millisecond, for up to a year and a half total duration.
{
	if (!dwTime)
	{
		//Stop timing.  Don't add anything to elapsed time.
		this->dwStartTime = 0;
		return;
	}

	if (!this->dwStartTime)
	{
		//Start timing.
		this->dwStartTime = dwTime;
		return;
	}

	//Add to time elapsed.
	static const UINT AFK_THRESHOLD = 5 * 60 * 1000; //don't add more time than this for one move (5 minutes)
	UINT elapsed = dwTime - this->dwStartTime;
	if (elapsed > AFK_THRESHOLD)
		elapsed = AFK_THRESHOLD;
	this->pPlayer->st.totalTime += elapsed;
	this->dwStartTime = dwTime;
}

//*****************************************************************************
bool CCurrentGame::WalkDownStairs()
//Move swordsman one step down/up stairs.
//
//Returns: whether the end of the stairs have been reached.
{
	//Decide whether to walk up or down the stairs.
	const UINT wOSquare = pRoom->GetOSquare(this->pPlayer->wX, this->pPlayer->wY);
	if (!bIsStairs(wOSquare)) return false; //not on stairs at all
	const int yOffset = wOSquare == T_STAIRS ? 1 : -1;
	if (this->pRoom->IsValidColRow(this->pPlayer->wX, this->pPlayer->wY + yOffset))
	{
		if (pRoom->GetOSquare(this->pPlayer->wX, this->pPlayer->wY + yOffset) == wOSquare)
		{
			++this->wTurnNo;  //to animate swordsman walking down each step in front end
			SetPlayer(this->pPlayer->wX, this->pPlayer->wY + yOffset);
			return true;
		}
	}

	return false;
}

//***************************************************************************************
/*
UINT CCurrentGame::WriteCurrentRoomDieDemo()
//Writes a demo to show player dieing in this room.
//
//Returns:
//DemoID of new Demos record.
{
	//Set recording information for a conquer demo.
	DEMO_REC_INFO dri;
	dri.dwDescriptionMessageID = MID_DieDemoDescription;
	dri.wBeginTurnNo = 0;
	dri.dwPrevDemoID = 0L;
	dri.dwFirstDemoID = 0L;
	dri.SetFlag(CDbDemo::Death);

	//This call does the real work.
	return WriteCurrentRoomDemo(dri);
}
*/

//
//Private methods.
//

//***************************************************************************************
void CCurrentGame::AddCompletedScripts()
//Remove NPCs for scripts that are marked "End"ed.
//Also, restart any NPC scripts flagged to do so.
{
	//Since rooms don't reset, we don't need to keep track of which NPCs ended
	//in previous room sessions.  Instead, just remove them from the room now on exit.
	//
	//Handling it this way is also necessary to avoid generated NPCs with a
	//colliding scriptID of another NPC in the hold from ending that NPC's script.
	CMonster *pMonster = this->pRoom->pFirstMonster;
	while (pMonster)
	{
		CMonster *pNext = pMonster->pNext;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->bScriptDone)
			{
				//Delete monster from room before room data is saved.
				this->pRoom->UnlinkMonster(pCharacter);
				this->pRoom->DeadMonsters.push_back(pCharacter); //don't dealloc yet -- might still have pointer refs
			}
		}
		pMonster = pNext;
	}

/*
	//Check for scripts that completed but were not marked yet.
	//This could happen for scripts that execute and complete on room entrance,
	//and then the player immediately leaves the room without making a move.
	CMonster *pMonster = this->pRoom->pFirstMonster;
	while (pMonster)
	{
		if (M_CHARACTER == pMonster->wType)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->bScriptDone)
				this->CompletedScriptsPending += pCharacter->dwScriptID;
		}
		pMonster = pMonster->pNext;
	}

	this->CompletedScripts += this->CompletedScriptsPending;
*/

	CDbSavedGame::removeGlobalScripts(this->CompletedScriptsPending);
	this->CompletedScriptsPending.clear();

	//Restart global scripts that are flagged to do so.
	for (pMonster = CDbSavedGame::pMonsterList; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsRestartScriptOnRoomEntrance())
				pCharacter->RestartScript();
		}
	}
}

//*****************************************************************************
void CCurrentGame::AddRoomsPreviouslyExploredByPlayerToMap(
	UINT playerID, const bool bMakeRoomsVisible) //[default=0, true]
{
	ASSERT(this->pHold);
	if (!playerID) {
		playerID = g_pTheDB->GetPlayerID();
		if (!playerID)
			return;
	}

	CIDSet roomsExplored;
	CDbHolds::GetRoomsExplored(this->pHold->dwHoldID, playerID, roomsExplored);
	roomsExplored -= this->dwRoomID;
	for (CIDSet::const_iterator it=roomsExplored.begin(); it!=roomsExplored.end(); ++it)
	{
		AddRoomToMap(*it, bMakeRoomsVisible, false);
	}
}

//***************************************************************************************
void CCurrentGame::AddRoomsToPlayerTally()
//Mark rooms that have been explored and conquered to player's total tally.
{
	if (this->bNoSaves)
		return;

/*
	if (GetAutoSaveOptions() <= ASO_CHECKPOINT) //implies playtesting or no saving
		return; //don't save the following record
*/
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwPlayerID);
	CIDSet exploredRooms(GetExploredRooms());
	exploredRooms += this->dwRoomID;
	g_pTheDB->SavedGames.AddRoomsToPlayerTally(dwPlayerID, exploredRooms);
}

//***************************************************************************************
void CCurrentGame::AddQuestionsToList(
//Adds questions from CID_MonsterSpoke event to a list of questions.
//
//Params:
	CCueEvents &CueEvents,              //(in)   May contain CID_MonsterSpoke and 
											//    associated questions.
	list<CMonsterMessage> &QuestionList)   //(out)  Questions will be added to it.
const
{
	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_MonsterSpoke);
	while (pObj)
	{
		const CMonsterMessage *pMessage = DYN_CAST(const CMonsterMessage*, const CAttachableObject*, pObj);
		if (pMessage->eType == MMT_YESNO || pMessage->eType == MMT_MENU)
			QuestionList.push_back(*pMessage);
		pObj = CueEvents.GetNextPrivateData();
	}
}

//*****************************************************************************
void CCurrentGame::AmbientSoundTracking(CCueEvents &CueEvents)
//Keep track of which persistent (looping) ambient sounds are playing on this turn.
//Also maintains a log of speech events.
{
	const CAttachableObject* pObj = CueEvents.GetFirstPrivateData(CID_AmbientSound);
	while (pObj)
	{
		const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		//wX/Y: sound location, wO: DataID (0 = stop all), wValue: loop if set
		if (!pCoord->wO)
		{
			//Stop ambient sounds.
			if (!this->pRoom->IsValidColRow(pCoord->wX, pCoord->wY))
				this->ambientSounds.clear(); //stop all ambient sounds
			else {
				//Remove sounds at indicated coordinate.
				vector<CMoveCoordEx> continuing;
				for (vector<CMoveCoordEx>::const_iterator ac = this->ambientSounds.begin();
						ac != this->ambientSounds.end(); ++ac)
				{
					if (ac->wX != pCoord->wX || ac->wY != pCoord->wY)
						continuing.push_back(*ac); //this sound is still playing
				}
				this->ambientSounds = continuing; //keep only sounds still playing
			}
		}
		else if (pCoord->wValue)
			this->ambientSounds.push_back(*pCoord);
		pObj = CueEvents.GetNextPrivateData();
	}

	//Speech log.
	pObj = CueEvents.GetFirstPrivateData(CID_Speech);
	while (pObj)
	{
		const CFiredCharacterCommand *pCmd = DYN_CAST(const CFiredCharacterCommand*,
				const CAttachableObject*, pObj);
		if (!pCmd->bFlush) //Ignore flush commands.
		{
			CCharacterCommand *pCommand = new CCharacterCommand(*(pCmd->pCommand));
			ASSERT(pCommand->pSpeech);
			pCommand->pSpeech->MessageText = pCmd->text.c_str(); //get interpolated text
			UINT& characterType = pCommand->pSpeech->wCharacter;
			if (characterType == Speaker_Self)
			{
				//Resolve now because there won't be any hook to the executing NPC later.
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pCmd->pExecutingNPC);
				characterType = pCharacter->wLogicalIdentity;

				//Convert to the speaker type.
				if (characterType < CUSTOM_CHARACTER_FIRST)
					characterType = getSpeakerType(MONSTERTYPE(characterType));
			}
			else if (characterType == Speaker_Player)
			{
				//If player is a custom type, set the character type to the player's
				//identity so the custom character's proper name will display
				//in the speech log.
				const UINT wPlayerIdentity = this->pPlayer->GetIdentity();
				if (wPlayerIdentity >= CUSTOM_CHARACTER_FIRST)
					characterType = wPlayerIdentity;
			}

			this->roomSpeech.push_back(pCommand);
		}
		pObj = CueEvents.GetNextPrivateData();
	}
}

//*****************************************************************************
void CCurrentGame::DeleteLeakyCueEvents(CCueEvents &CueEvents)
//Deletes objects allocated on the heap and connected to cue events
//that must be deleted by a handler, but aren't handled in this instance.
{
	static const UINT eventsRequiringDeletion = 1;
	static const CUEEVENT_ID events[eventsRequiringDeletion] = {CID_Speech};

	const CAttachableObject *pObj;
	for (UINT i=0; i<eventsRequiringDeletion; ++i)
		for (pObj = CueEvents.GetFirstPrivateData(events[i]); pObj != NULL;
				pObj = CueEvents.GetNextPrivateData())
			delete pObj;
}

//*****************************************************************************
/*
void CCurrentGame::DrankPotion(CCueEvents &CueEvents, const UINT wDoubleType)
//Drink potion, begin double placement and init double cursor position.
{
   this->pPlayer->wPlacingDoubleType=wDoubleType;
   this->pPlayer->wDoubleCursorX=this->pPlayer->wX;
   this->pPlayer->wDoubleCursorY=this->pPlayer->wY;
   this->pRoom->Plot(this->pPlayer->wX, this->pPlayer->wY, T_EMPTY);
	CueEvents.Add(CID_DrankPotion);
}

//-****************************************************************************
void CCurrentGame::FegundoToAsh(CMonster *pMonster, CCueEvents &CueEvents)
//Fegundo has exploded.  Remove it and replace it with ashes.
{
	const UINT wX = pMonster->wX;
	const UINT wY = pMonster->wY;
	if (pMonster->bAlive) //if a resulting bomb explosion did not also kill the fegundo
	{
		this->pRoom->KillMonster(pMonster, CueEvents);

		//Add ashes if room square doesn't forbid it.
		const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
		switch (wOTile)
		{
			case T_PIT: case T_PIT_IMAGE:
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wX, wY,
					S, M_OFFSET + M_FEGUNDOASHES), true);
			break;
			case T_WATER:
				CueEvents.Add(CID_Splash, new CCoord(wX,wY), true);
			break;
			default:
				CMonster *pAshes = this->pRoom->AddNewMonster(M_FEGUNDOASHES,wX,wY);
				pAshes->bIsFirstTurn = true;
				if (wOTile == T_PRESSPLATE)
					this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);
			break;
		}
	}
}

//-****************************************************************************
UINT CCurrentGame::FindLastCheckpointSave(
//Play back stored commands to change the game state.
//Assumes that the room has been freshly loaded.
//
//Params:
	const UINT wSearchUntilTurn) //(in) search until this turn no.
//
//Returns:
//Turn of last checkpoint save made in this room, or 0 if none.
{
	ASSERT(this->wTurnNo == 0);
	ASSERT(wSearchUntilTurn <= this->Commands.Count());

	//If there are no commands, we're at the beginning of the room - return 0
	if (this->Commands.Empty()) return 0;

	//While processing the command list, I don't want to take any actions that
	//will modify the command list.
	FreezeCommands();

//	UINT wLastCheckpointSave = 0;

	CDbCommands::const_iterator comIter = this->Commands.GetFirst();
	CCueEvents CueEvents;
	UINT wCommandI, wX=(UINT)-1, wY=(UINT)-1;
	for (wCommandI = 0; wCommandI < wSearchUntilTurn-1; ++wCommandI)
	{
		ASSERT(comIter != this->Commands.end());
		
		const int nCommand = comIter->bytCommand;
		if (bIsComplexCommand(nCommand))	//handle multi-part commands here
			VERIFY(this->Commands.GetData(wX,wY));
		ProcessCommand(nCommand, CueEvents, wX, wY);
		if (CueEvents.HasOccurred(CID_CheckpointActivated))
			wLastCheckpointSave = wCommandI+1;

		//Check for unexpected game states that indicate commands are invalid.
		//Note: these should never happen, since we're examining the current game in play.
		ASSERT(this->bIsGameActive);
		ASSERT(!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied));  //player shouldn't have died
		ASSERT(!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom));

		comIter = this->Commands.GetNext();
	}

	//Allow modification of command list again.
	UnfreezeCommands();

	return wLastCheckpointSave;
}
*/

//*****************************************************************************
bool CCurrentGame::KnockOnDoor(CCueEvents& CueEvents, const UINT wX, const UINT wY)
//Open door by bumping into it from floor level.
//
//Returns: whether door may be opened
{
	//Does player have a key to open this door type?
	PlayerStats& ps = this->pPlayer->st;
	const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
	switch (wOTile)
	{
		case T_DOOR_Y:
			if (ps.yellowKeys)
			{
				--ps.yellowKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, YellowKey), true);
			} else if (ps.skeletonKeys) {
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, SkeletonKey), true);
			}
			else
				return false;
		break;
		case T_DOOR_G:
			if (ps.greenKeys)
			{
				--ps.greenKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, GreenKey), true);
			} else if (ps.skeletonKeys) {
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, SkeletonKey), true);
			}
			else
				return false;
		break;
		case T_DOOR_C:
			if (ps.blueKeys)
			{
				--ps.blueKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, BlueKey), true);
			} else if (ps.skeletonKeys) {
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, SkeletonKey), true);
			}
			else
				return false;
		break;
		case T_DOOR_MONEY:
		{
			const int cost = getItemAmount(wOTile);
			if (ps.GOLD >= cost)
			{
				incintValueWithBounds(ps.GOLD, -cost); //gold may go negative
				CueEvents.Add(CID_EntityAffected, new CCombatEffect(this->pPlayer, CET_GOLD, -cost), true);
			} else if (ps.skeletonKeys) {
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, SkeletonKey), true);
			}
			else
				return false;

			CueEvents.Add(CID_MoneyDoorOpened);

			this->pRoom->OpenDoor(wX, wY);
			return true;
		}
		break;
		default: return false;
	}

	CueEvents.Add(CID_KnockOpenedDoor, NULL, false);
	this->pRoom->OpenDoor(wX, wY);
	return true;
}

//*****************************************************************************
bool CCurrentGame::LockDoor(CCueEvents& CueEvents, const UINT wX, const UINT wY)
//The lock command on a door will close it, expending the appropriate resource.
//
//Returns: whether door was closed
{
	//Does player have a key to operate this door type?
	PlayerStats& ps = this->pPlayer->st;
	const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
	switch (wOTile)
	{
		case T_DOOR_YO:
			if (ps.yellowKeys)
			{
				--ps.yellowKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, YellowKey), true);
			}
			else if (ps.skeletonKeys)
			{
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY - (wY > 0 ? 1 : -1), SkeletonKey), true);
			}
			else
				return false;
		break;
		case T_DOOR_GO:
			if (ps.greenKeys)
			{
				--ps.greenKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, GreenKey), true);
			}
			else if (ps.skeletonKeys)
			{
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY - (wY > 0 ? 1 : -1), SkeletonKey), true);
			}
			else
				return false;
		break;
		case T_DOOR_CO:
			if (ps.blueKeys)
			{
				--ps.blueKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY, BlueKey), true);
			}
			else if (ps.skeletonKeys)
			{
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY - (wY > 0 ? 1 : -1), SkeletonKey), true);
			}
			else
				return false;
		break;

		case T_DOOR_MONEYO:
		{
			const int cost = getItemAmount(wOTile);
			if (ps.GOLD >= cost)
			{
				incintValueWithBounds(ps.GOLD, -cost); //gold may go negative
				CueEvents.Add(CID_EntityAffected, new CCombatEffect(this->pPlayer, CET_GOLD, -cost), true);
			} else if (ps.skeletonKeys) {
				--ps.skeletonKeys;
				CueEvents.Add(CID_ItemUsed, new CMoveCoord(wX, wY - (wY > 0 ? 1 : -1), SkeletonKey), true);
			}
			else
				return false;

			CueEvents.Add(CID_MoneyDoorLocked);

			this->pRoom->CloseDoor(wX, wY);
		}
		return true; //done

		default: return false;
	}

	CueEvents.Add(CID_DoorLocked, NULL, false);
	this->pRoom->CloseDoor(this->pPlayer->wX, this->pPlayer->wY);
	return true;
}

//*****************************************************************************
void CCurrentGame::LoadNewRoomForExit(
//Loads room adjacent to the current room in the direction of the swordsman's
//exit of the current room.  Swordsman coords are also updated so that he wraps to
//other side in new room.
//
//Params:
	const UINT dwSX, const UINT dwSY,
	CDbRoom* pNewRoom,
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first step 
	                        //  into the room.
//	const bool bSaveGame)   //save game if set
{
	ASSERT(pNewRoom);
	delete this->pRoom;
	this->pRoom = pNewRoom;

	//Put swordsman at designated square.
	SetPlayer(dwSX, dwSY);

	//Set start room members.
	SetRoomStartToPlayer();
	SetPlayerToRoomStart(); //reset some vars so saving game works properly

	SetMembersAfterRoomLoad(CueEvents);
}

//***************************************************************************************
bool CCurrentGame::LoadEastRoom()
//Loads room east of the current room in the context of the active entity exiting
//from the east end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX + 1, this->pRoom->dwRoomY))
		return false;

	//Put player at west edge of new room.
	SetPlayer(0, this->pPlayer->wY);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadNorthRoom()
//Loads room north of the current room in the context of the active entity exiting
//from the north end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX, this->pRoom->dwRoomY - 1))
		return false;

	//Put player at south edge of new room.
	SetPlayer(this->pPlayer->wX, this->pRoom->wRoomRows - 1);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadSouthRoom()
//Loads room south of the current room in the context of the active entity exiting
//from the south end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX, this->pRoom->dwRoomY + 1))
		return false;

	//Put player at north edge of new room.
	SetPlayer(this->pPlayer->wX, 0);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadWestRoom()
//Loads room west of the current room in the context of the active entity exiting
//from the west end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX - 1, this->pRoom->dwRoomY))
		return false;

	//Put player at east edge of new room.
	SetPlayer(this->pRoom->wRoomCols - 1, this->pPlayer->wY);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadAdjoiningRoom(
//Returns: whether there is an adjoining room in the direction of exit
//
//OUT: Loads room adjacent to the current room in the direction of
//exit from the current room.  Coords are also outputted of the tile to start on
//in new room.
//
//Params:
	const UINT wDirection,  //(in) direction player is leaving room from
	UINT &dwNewSX, UINT &dwNewSY,  //(out) destination room entrance tile
	CDbRoom* &pNewRoom)
{
	pNewRoom = NULL;

	//Player should be exiting from proper edge of room.
	//Get coordinates of room being entered.
	UINT dwNewRoomX, dwNewRoomY;
	switch (wDirection)
	{
		case N:
			ASSERT(this->pPlayer->wY == 0);
			dwNewRoomX = this->pRoom->dwRoomX;
			dwNewRoomY = this->pRoom->dwRoomY - 1;
			break;
		case S:
		   ASSERT(this->pPlayer->wY == this->pRoom->wRoomRows - 1);
			dwNewRoomX = this->pRoom->dwRoomX;
			dwNewRoomY = this->pRoom->dwRoomY + 1;
			break;
		case W:
			ASSERT(this->pPlayer->wX == 0);
			dwNewRoomX = this->pRoom->dwRoomX - 1;
			dwNewRoomY = this->pRoom->dwRoomY;
			break;
		case E:
		   ASSERT(this->pPlayer->wX == this->pRoom->wRoomCols - 1);
			dwNewRoomX = this->pRoom->dwRoomX + 1;
			dwNewRoomY = this->pRoom->dwRoomY;     
			break;
		default:
			ASSERT(!"Bad direction value.");
		return false;
	}

	//Attempt to load room.
	pNewRoom = this->pLevel->GetRoomAtCoords(dwNewRoomX, dwNewRoomY);
	if (!pNewRoom)
		return false;

	//Put player at opposite edge of new room.
	switch (wDirection)
	{
		case N:
			dwNewSX = this->pPlayer->wX;
			dwNewSY = pNewRoom->wRoomRows - 1;
			break;
		case S:
			dwNewSX = this->pPlayer->wX;
			dwNewSY = 0;      
			break;
		case W:
			dwNewSX = this->pRoom->wRoomCols - 1;
			dwNewSY = this->pPlayer->wY;    
			break;
		case E:
			dwNewSX = 0;
			dwNewSY = this->pPlayer->wY;    
			break;
	}

	//Determine whether player can enter room here.
//	const UINT wTurnNo_ = this->wTurnNo; //need to reset the turn number when
//	this->wTurnNo = 0; //calling SetCurrentGame to indicate initial default NPC state must be set
	pNewRoom->SetCurrentGame(this);
//	this->wTurnNo = wTurnNo_;

	RetrieveExploredRoomData(*pNewRoom);
	return true;
}

//***************************************************************************************
void CCurrentGame::ProcessMonsters(
//Processes all the monsters in the current room.
//
//Params:
	int nLastCommand,    //(in)      Last swordsman command.
	CCueEvents &CueEvents)  //(in/out)  List of events that can be handled by caller.
{
	CMonster *pMonster, *pNextMonster;
//	UINT wX, wY, wO;

	if (!this->bHalfTurn)
	{
		//Increment the spawn cycle counter.
		++this->wSpawnCycleCount;
	}

/*
	PreprocessMonsters(CueEvents);

	//Brains don't affect monster movement in this game.
	this->bBrainSensesSwordsman = false; //this->pRoom->BrainSensesSwordsman();
*/

	//Since player and mimic movement is practically synched,
	//wait until all mimics have moved, possibly moving platforms, before
	//things can fall down.
	bool bMimicsMoved = this->pRoom->platforms.empty(); //for platform

	//Each iteration processes one monster.
	for (pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pNextMonster)
	{
		//No more monsters should move, including processing NPC scripts, if
		//an event to exit the level has already occurred.
		//In this event, the room data have already been packed and NPC script
		//state saved, and further processing could get monsters and packed
		//state out of synch when the room is loaded again.
		ASSERT(!CueEvents.HasOccurred(CID_ExitLevelPending));

		//Platforms fall when mimics are done moving.
		const bool bIsMimic = pMonster->wType == M_MIMIC;
		if (!bMimicsMoved && !bIsMimic)
		{
			CPlatform::checkForFalling(this->pRoom, CueEvents);
			bMimicsMoved = true;
		}

		if (pMonster->bIsFirstTurn || !pMonster->IsAlive())
		{
			//Skip monster on its first turn.
			pNextMonster = pMonster->pNext;
		} else {
			//Monster is moving from its current position.
			pMonster->wPrevX = pMonster->wX;
			pMonster->wPrevY = pMonster->wY;
			pMonster->wPrevO = pMonster->wO;

			//Skip monster's move when performing a speed move
			//(Mimics get to move as they're synchronized with the player.)
			if (!this->bHalfTurn || bIsMimic)
			{
				//Process monster.
				pMonster->Process(nLastCommand, CueEvents);

				if (CueEvents.HasOccurred(CID_ExitRoom))
					return; //if monster caused a room exit, then don't process anything else this turn

				//Check for damage inflicted from something in the room.
				if (pMonster->bAlive)
				{
					if (pMonster->CheckForDamage(CueEvents))
					{
						pNextMonster = pMonster->pNext;
						this->pRoom->KillMonster(pMonster, CueEvents);
/*
						TallyKill();
						if (pMonster->wType == M_ROCKGIANT)
							CSplitter::Shatter(CueEvents, this, pMonster->wX, pMonster->wY);
*/
						continue;	//extra processing below gets skipped on death
					}
				}
			} else {
				//Handle monsters that perform specific functions on half-turns.
				switch (pMonster->wType)
				{
/*
					case M_EYE:
					{
						//Do evil eyes wake up?
						CMoveCoord *pCoord;
						int nOX, nOY;
						CEvilEye *pEye = DYN_CAST(CEvilEye*, CMonster*, pMonster);
						pEye->WakeupCheck(CueEvents, pCoord, nOX, nOY);
					}
					break;
*/
					case M_CHARACTER:
					{
						//Character commands that don't expend a turn processed.
						this->bExecuteNoMoveCommands = true;
						pMonster->Process(nLastCommand, CueEvents);
						this->bExecuteNoMoveCommands = false;
					}
					break;
					default: break; //all other monster types do nothing
				}
			}

			//Remember the next monster now, because this monster may be dead and
			//removed from the monster list in the next block.
			pNextMonster = pMonster->pNext;

			//Check for events stemming from the monster's behavior
			//which require modification of the monster list.
			switch (pMonster->wType)
			{
				case M_MIMIC: case M_CLONE: case M_DECOY: case M_STALWART:
				case M_SLAYER: case M_GUARD: case M_PIRATE:
				{
					//Process double's sword hit.
					CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
					pDouble->SetSwordSheathed();
					if (pDouble->HasSword())
					{
						ProcessSwordHit(pDouble->GetSwordX(), pDouble->GetSwordY(), CueEvents, pDouble);
						//Update next monster pointer, because the sword hit may have removed
						//the next monster.
						pNextMonster = pMonster->pNext;
					}
				}
				break;
				case M_CHARACTER:
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					//Mark when script is completed.
					if (pCharacter->bScriptDone)
						this->CompletedScriptsPending += pCharacter->dwScriptID;
					if (pCharacter->IsVisible())
					{
						pCharacter->SetSwordSheathed();

						//Rules for NPC making a sword strike:
						const UINT swordType = pCharacter->getSword();
						if (pCharacter->HasSword() && //1. NPC sword is not sheathed/disabled
								swordType != NoSword && //2. NPC is not set to have "no sword"
								(swordType != NPC_DEFAULT_SWORD || //3a. NPC either doesn't have their default sword type...
										bEntityHasSword(pCharacter->GetIdentity()))) //3b.  ...or if they do, they are a sworded monster type by default
										//Rule 3 is important to make the distinction between implicitly sworded and swordless NPC monster types
										//when their sword state is set to the default.
						{
							ProcessSwordHit(pCharacter->GetSwordX(), pCharacter->GetSwordY(), CueEvents, pMonster);
							//Update next monster pointer, because the sword hit may have removed 
							//the next monster.
							pNextMonster = pMonster->pNext;
						}
					}
/* empty block
					if (pCharacter->bReplaced)
					{
						//Character "reverted" to the monster matching its appearance.
						CMonster *pMonster = this->pRoom->GetMonsterAtSquare(
								pCharacter->wX, pCharacter->wY);
						ASSERT(pMonster);

						//The new monster might require changes to room state (e.g. pathmapping).
//				      this->pRoom->CreatePathMaps();

/-*
						if (CueEvents.HasOccurredWith(CID_FegundoToAsh, pMonster))
							FegundoToAsh(pMonster, CueEvents);
* /
					}
*/
					if (pCharacter->bGlobal)
					{
						//Remove NPC object to global script list.
						this->pRoom->UnlinkMonster(pCharacter);

						appendToGlobalMonsterList(pCharacter);
					}

					//If this NPC has fired an exit level event, then room data have
					//already been packed, and processing further NPC scripts
					//could get the NPC state out of synch with the packed NPC vars,
					//causing confusion on room load.
					//So -- don't process any more NPC scripts for this turn.
					//Note that all other monster types have taken their turn by
					//the time NPCs are processed in the monster list,
					//so there's no danger of other monster types being skipped.
					if (CueEvents.HasOccurred(CID_ExitLevelPending))
						pNextMonster = NULL;
				}
				break;
				case M_SERPENT:
					if (CueEvents.HasOccurredWith(CID_SnakeDiedFromTruncation, pMonster))
					{
						// Red serpent died from shortening.
						this->pRoom->KillMonster(pMonster, CueEvents);
//						TallyKill();  //counts as a kill
					}
				break;
/*
				case M_REGG:
					if (CueEvents.HasOccurredWith(CID_EggHatched, pMonster))
					{
						// Spawn a roach AFTER egg has been removed.
						wX = pMonster->wX;
						wY = pMonster->wY;
						this->pRoom->KillMonster(pMonster, CueEvents);
						CMonster *m = this->pRoom->AddNewMonster(M_ROACH,wX,wY);
						m->bIsFirstTurn = true;
					}
				break;
				case M_FEGUNDO:
					if (CueEvents.HasOccurredWith(CID_FegundoToAsh, pMonster))
						FegundoToAsh(pMonster, CueEvents);
				break;
				case M_FEGUNDOASHES:
					if (CueEvents.HasOccurredWith(CID_AshToFegundo, pMonster))
					{
						// Spawn a fegundo AFTER ash has been removed.
						wX = pMonster->wX;
						wY = pMonster->wY;
						wO = this->pPlayer->wO;
						this->pRoom->KillMonster(pMonster, CueEvents);
						CMonster *m = this->pRoom->AddNewMonster(M_FEGUNDO,wX,wY);
						m->SetOrientation(nGetOX(wO), nGetOY(wO));
						m->bIsFirstTurn = true;
					}
				break;
*/
			}
		}
	}
}

//***************************************************************************************
void CCurrentGame::ProcessTokenActivations(CCueEvents &CueEvents)
//Since multiple monsters can step on tokens on a turn, causing weird effects
//due to movement order, we will hold off on activating tokens until all monsters
//have moved, and then perform all token activations at once.
//This should eliminate token idiosyncrasies due to monster movement order.
{
	for (vector<CCoord>::const_iterator token = this->pendingTokenActivations.begin();
			token != this->pendingTokenActivations.end(); ++token)
	{
		//Activate token now.
		const CCoord& coord = *token;
		this->pRoom->ActivateToken(CueEvents, coord.wX, coord.wY);
	}

	this->pendingTokenActivations.clear();
}

//***************************************************************************************
void CCurrentGame::ProcessSimultaneousSwordHits(
//Processes results of all swords (mimic, player, etc.) entering squares that
//must be stabbed simultaneously.
//
//Params:
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
{
	//NOTE: this is currently only relevant and in effect for tarstuff stabbings,
	//and not for other breakable object types

	for (vector<CMoveCoord>::const_iterator stab = this->simulSwordHits.begin();
			stab != this->simulSwordHits.end(); ++stab)
	{
		//Remove tarstuff now.
		const CMoveCoord& stabCoord = *stab;
		this->pRoom->StabTar(stabCoord.wX, stabCoord.wY, CueEvents, true, stabCoord.wO);
	}
	this->simulSwordHits.clear();
}

//***************************************************************************************
bool CCurrentGame::CanPlayerMoveTo(const UINT wX, const UINT wY) const
//Returns: whether player may step to (x,y) from current location
//
//NOTE: Matches general movement logic in ProcessPlayer
{
	const CSwordsman& p = *this->pPlayer;
	switch (nDist(p.wX, p.wY, wX, wY))
	{
		case 0: return true; //player can remain stationary
		case 1: break; //find out whether move is valid below
		default: return false; //player can't step more than one tile away
	}

	const int dx = int(wX) - int(p.wX), dy = int(wY) - int(p.wY);
	const UINT wMoveO = nGetO(dx, dy);
	CDbRoom& room = *this->pRoom;

	//Directional movement hindrances.
	const UINT wFTileNo = room.GetFSquare(p.wX, p.wY);
	if (bIsArrowObstacle(wFTileNo, wMoveO))
		return false;

	if (room.DoesSquarePreventDiagonal(p.wX, p.wY, dx,dy))
		return false;

	//Player enters a tunnel.
	const UINT wOTileNo = room.GetOSquare(p.wX, p.wY);
	if (bIsTunnel(wOTileNo)) // && p.IsTarget())
	{
		bool bEnteredTunnel = false;
		switch (wOTileNo)
		{
			case T_TUNNEL_N: if (wMoveO == N) bEnteredTunnel = true; break;
			case T_TUNNEL_S: if (wMoveO == S) bEnteredTunnel = true; break;
			case T_TUNNEL_E: if (wMoveO == E) bEnteredTunnel = true; break;
			case T_TUNNEL_W: if (wMoveO == W) bEnteredTunnel = true; break;
			default: ASSERT(!"Unrecognized tunnel type"); break;
		}
		if (bEnteredTunnel)
			return true;
	}

	//Player leaves the room.
	if (!room.IsValidColRow(wX,wY))
		return true;

	const UINT wNewFTile = room.GetFSquare(wX, wY);
	if (bIsArrowObstacle(wNewFTile, wMoveO) ||
			room.DoesSquarePreventDiagonal(wX, wY, dx,dy))
		return false;

	if (!room.DoesSquareContainPlayerObstacle(wX, wY, wMoveO,
			bIsElevatedTile(room.GetOSquare(p.wX, p.wY))))
		return true;

	//Specific obstacle types.
	const UINT destX = p.wX + dx, destY = p.wY + dy;
	bool bNotAnObstacle = false; //this gets set if an obstacle can actually be moved onto
	const UINT wNewOTile = room.GetOSquare(wX, wY);

	//Monsters can be moved through when invisible.
	CMonster *pMonster = room.GetMonsterAtSquare(wX, wY);
	if (pMonster)
	{
		if (p.IsInvisible())
		{
			bNotAnObstacle = true;
			goto CheckOLayer;
		}
		return false; //monster is in the way
	}

CheckOLayer:
	//Floor layer objects permit movement in specific contexts.
	switch (wNewOTile)
	{
		case T_PIT: case T_PIT_IMAGE:
			//If standing on a platform, check whether it can move.
			if (wOTileNo == T_PLATFORM_P)
				if (room.CanMovePlatform(p.wX, p.wY, wMoveO))
					goto CheckTLayer;

			//If player can jump a tile of pit, extend the movement direction.
			if (p.CanJump(dx,dy) && room.CanJumpTo(
					destX, destY, destX + dx, destY + dy,
					bIsElevatedTile(room.GetOSquare(p.wX, p.wY))))
			{
				bNotAnObstacle = true;
				goto CheckTLayer;
			}
		break;
		case T_WATER:
			if (wOTileNo == T_PLATFORM_W)
				if (room.CanMovePlatform(p.wX, p.wY, wMoveO))
				{
					bNotAnObstacle = true;
					goto CheckTLayer;
				}

			//If player can traverse water, water is not an obstacle.
			if (p.st.accessory == WaterBoots)
			{
				bNotAnObstacle = true;
				goto CheckTLayer;
			}

			//If player can jump a tile of water, extend the movement direction.
			if (p.CanJump(dx,dy) && room.CanJumpTo(
					destX, destY, destX + dx, destY + dy,
					bIsElevatedTile(room.GetOSquare(p.wX, p.wY))))
			{
				bNotAnObstacle = true;
				goto CheckTLayer;
			}
		break;
		case T_DOOR_Y:
		case T_DOOR_C: case T_DOOR_G:
		case T_DOOR_MONEY:
		case T_DOOR_B: case T_DOOR_R:
			//Player may walk on doors when standing on them.
			if (bIsDoor(wOTileNo))
			{
				bNotAnObstacle = true;
				goto CheckTLayer;
			}
		break;
		default:
			goto CheckTLayer;
		break;
	}
	return false; //a terrain listed above didn't allow movement in this case

CheckTLayer:
	const UINT wNewTTile = room.GetTSquare(wX, wY);
	switch (wNewTTile)
	{
		//Can't move through these items even if o-layer objects weren't obstacles.
		case T_ORB:
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
		break;

		//Can push mirror, if possible.
		case T_MIRROR:
			if (room.CanPlayerMoveOnThisElement(p.wAppearance,
					this->pRoom->GetOSquare(destX, destY),
					bIsElevatedTile(room.GetOSquare(p.wX, p.wY))) &&
						room.CanPushTo(destX, destY, destX + dx, destY + dy))
			{
				bNotAnObstacle = true;
				goto CheckMonsterLayer;
			}
		break;
		default:
			goto CheckMonsterLayer;
		break;
	}
	return false; //an object listed above didn't allow movement in this case

CheckMonsterLayer:
	//Don't allow path mapping through monster swords.
	if (room.IsMonsterSwordAt(destX, destY))
		return false;

	//If what can be obstacle permitted movement in this case,
	//and nothing else was an obstacle, then the player may move here.
	if (bNotAnObstacle)
		goto CanMakeMove;

	return false;

CanMakeMove:
	return true;
}

//***************************************************************************************
void CCurrentGame::ProcessPlayer(
//Processes a player command.
//
//Params:
	const int nCommand,        //(in)   Game command.
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
{
	CSwordsman& p = *this->pPlayer; //shorthand
	bool bMoved = false;
	const UINT wOldX = p.wX;
	const UINT wOldY = p.wY;

	int dx = 0, dy = 0;
	//Figure out how to change player based on command.
	switch (nCommand)
	{
		//Rotate player.
		case CMD_C:
			if (p.HasSword())
				CueEvents.Add(CID_SwingSword,
					new CAttachableWrapper<UINT>(p.wO), true);	//comes first
			p.RotateClockwise();
			break;
		case CMD_CC:
			p.RotateCounterClockwise();
			if (p.HasSword())
				CueEvents.Add(CID_SwingSword,
					new CAttachableWrapper<UINT>(p.wO), true);	//comes after
			break;
		//Move player.
		case CMD_NW: dx = dy = -1; break;
		case CMD_N: dy = -1; break;
		case CMD_NE: dx = 1; dy = -1; break;
		case CMD_W: dx = -1; break;
		case CMD_E: dx = 1; break;
		case CMD_SW: dx = -1; dy = 1; break;
		case CMD_S: dx = 0; dy = 1; break;
		case CMD_SE: dx = dy = 1; break;

		case CMD_LOCK:
		case CMD_WAIT: break;

		//Using an inventory item takes a turn like other movements.
		case CMD_USE_WEAPON:
		{
			//Process a custom weapon script.
			if (!MayUseItem(ScriptFlag::Weapon))
				break;
			CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Weapon);
			if (pCharacter)
				pCharacter->ProcessAfterUse(CueEvents);
		}
		break;
		case CMD_USE_ARMOR:
		{
			//Process a custom armor script.
			if (!MayUseItem(ScriptFlag::Armor))
				break;
			CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Armor);
			if (pCharacter)
				pCharacter->ProcessAfterUse(CueEvents);
		}
		break;
		case CMD_USE_ACCESSORY:
			bMoved = UseAccessory(CueEvents);
		break;

		case CMD_EXEC_COMMAND:
		{
			if (!MayUseItem(ScriptFlag::Command))
				break;
			CCharacter *pCharacter = getCustomEquipment(ScriptFlag::Command);
			if (pCharacter)
				pCharacter->ProcessAfterUse(CueEvents);
		}
		break;

		default: return; //invalid command -- might be from an old version demo
	}

	const UINT nFirstO = nGetO(dx,dy);

/*
	//Player can take steps if not frozen.
	if (this->pPlayer->bFrozen)
		dx = dy = 0;
*/

	//Calculate sword movement
	p.wSwordMovement = CSwordsman::GetSwordMovement(nCommand, p.wO);

	CDbRoom& room = *this->pRoom;
	const UINT wOTileNo = room.GetOSquare(wOldX, wOldY);
	const UINT wFTileNo = room.GetFSquare(wOldX, wOldY);
	const UINT wTTileNo = room.GetTSquare(wOldX, wOldY);
	bool bEnteredTunnel = false;
	bool bMovingPlatform = false;
	bool bJumping=false, bSwimming=false;

	//Look for obstacles and set dx/dy accordingly.
	const UINT wMoveO = nGetO(dx, dy);

	if (!dx && !dy)
		goto MakeMove;

	//Check for arrows underfoot.
	if (bIsArrowObstacle(wFTileNo, wMoveO))
	{
		CueEvents.Add(CID_HitObstacle, new CMoveCoord(
				wOldX, wOldY, wMoveO), true);
		dx = dy = 0;
		goto MakeMove;
	}
	if (room.DoesSquarePreventDiagonal(
			wOldX, wOldY, dx,dy))
	{
		CueEvents.Add(CID_HitObstacle, new CMoveCoord(
			wOldX + (wFTileNo == T_NODIAGONAL ? 0 : dx),
			wOldY + (wFTileNo == T_NODIAGONAL ? 0 : dy), wMoveO), true);
		dx = dy = 0;
		goto MakeMove;
	}

	//Check for tunnel entrance before checking for room exit.
	//Player enters tunnel when moving off of a tunnel in its entrance direction.
	if (bIsTunnel(wOTileNo)) // && p.IsTarget())
	{
		switch (wOTileNo)
		{
			case T_TUNNEL_N: if (wMoveO == N) bEnteredTunnel = true; break;
			case T_TUNNEL_S: if (wMoveO == S) bEnteredTunnel = true; break;
			case T_TUNNEL_E: if (wMoveO == E) bEnteredTunnel = true; break;
			case T_TUNNEL_W: if (wMoveO == W) bEnteredTunnel = true; break;
			default: ASSERT(!"Unrecognized tunnel type"); break;
		}
		if (bEnteredTunnel)
			goto MakeMove;
	}

	//Check for leaving room.
	if (!room.IsValidColRow(wOldX + dx, wOldY + dy))
	{
/*
		//Prevent player from leaving room when lock is enabled.
		if (this->bRoomExitLocked)
		{
			CueEvents.Add(CID_RoomExitLocked);
			return;
		}
*/
		if (ProcessPlayer_HandleLeaveRoom(wMoveO, CueEvents))
			return;

		//Probably a room that had an exit but no room adjacent
		//in that direction, or cannot enter room on that square.
		//Just treat it as an obstacle.
		dx = dy = 0;
		goto MakeMove;
	}

/*
	//Prevent player from leaving room via stairs when lock is enabled.
	if (this->bRoomExitLocked)
	{
		//Don't check whether move is legal -- just mention to player stepping
		//onto stairs is currently locked.
		if (bIsStairs(room.GetOSquare(this->swordsman.wX + dx, this->swordsman.wY + dy)))
		{
			CueEvents.Add(CID_RoomExitLocked);
			return;
		}
	}
*/

	//Check for obstacles in destination square.
	if (room.DoesSquareContainPlayerObstacle(
			wOldX + dx, wOldY + dy, wMoveO, bIsElevatedTile(room.GetOSquare(wOldX, wOldY))))
	{
		//There is an obstacle on the destination square,
		//but it might need special handling or queries.
		//Check each layer for an object that gets handled specially.
		const UINT wNewOTile = room.GetOSquare(wOldX + dx,
				wOldY + dy);
		const UINT wNewFTile = room.GetFSquare(wOldX + dx,
				wOldY + dy);
		const UINT wNewTTile = room.GetTSquare(wOldX + dx,
				wOldY + dy);
		CMonster *pMonster = room.GetMonsterAtSquare(
				wOldX + dx, wOldY + dy);
		bool bNotAnObstacle=false, bPushingMirror=false;

		//If a monster is on the destination tile, player fights it,
		//even if player won't be able to move onto that tile once the monster is gone.
		if (pMonster)
		{
			ASSERT(!InCombat());

			bool bInitiateCombat = true;
			switch (pMonster->wType)
			{
				case M_CHARACTER:
				{
					//Player bumps into an NPC or walks on an invisible NPC.
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					pCharacter->bPlayerTouchedMe = true;
					if (pCharacter->IsSafeToPlayer())
						bInitiateCombat = false; //player cannot fight safe monsters
				}
				break;
				default: break;
			}

			if (p.IsInvisible()) //if invisible, combat doesn't occur
				goto CheckOLayer;

			if (bInitiateCombat)
				InitiateCombat(CueEvents, pMonster,
						false, //monster hits player first when player doesn't use a sword to attack
						wOldX, wOldY,
						wOldX + dx, wOldY + dy);


			dx = dy = 0;
			goto MakeMove;
		}

CheckOLayer:
		//Check o-layer stuff.
		switch (wNewOTile)
		{
			case T_PIT: case T_PIT_IMAGE:
				//If standing on a platform, check whether it can move.
				if (wOTileNo == T_PLATFORM_P)
					if (room.CanMovePlatform(wOldX, wOldY, nFirstO))
					{
						bMovingPlatform = bNotAnObstacle = true;
						goto CheckFLayer;
					}

				//If player can jump a tile of pit, extend the movement direction.
				if (p.CanJump(dx,dy) && room.CanJumpTo(
						wOldX + dx, wOldY + dy,
						wOldX + dx*2, wOldY + dy*2,
						bIsElevatedTile(room.GetOSquare(wOldX, wOldY))))
				{
					bJumping = bNotAnObstacle = true;
					dx *= 2;
					dy *= 2;
					goto CheckFLayer;
				}

				CueEvents.Add(CID_Scared);
			break;
			case T_WATER:
				if (wOTileNo == T_PLATFORM_W)
					if (room.CanMovePlatform(wOldX, wOldY, nFirstO))
					{
						bMovingPlatform = bNotAnObstacle = true;
						goto CheckFLayer;
					}

				//If player can traverse water, water is not an obstacle.
				if (p.st.accessory == WaterBoots)
				{
					bNotAnObstacle = bSwimming = true;
					goto CheckFLayer;
				}

				//If player can jump a tile of water, extend the movement direction.
				if (p.CanJump(dx,dy) && room.CanJumpTo(
						wOldX + dx, wOldY + dy,
						wOldX + dx*2, wOldY + dy*2,
						bIsElevatedTile(room.GetOSquare(wOldX, wOldY))))
				{
					bJumping = bNotAnObstacle = true;
					dx *= 2;
					dy *= 2;
					goto CheckFLayer;
				}

				CueEvents.Add(CID_PlayerOnWaterEdge);
			break;
			case T_DOOR_Y:
			case T_DOOR_C: case T_DOOR_G:
			case T_DOOR_MONEY:
				//When adjacent a door, player may knock on it to open it.
				if (!bIsElevatedTile(wOTileNo))
				{
					if (KnockOnDoor(CueEvents, wOldX + dx, wOldY + dy))
						break;
				}
			//NO BREAK
			case T_DOOR_B: case T_DOOR_R:
				//Player may walk on doors when standing on them, but is not allowed to knock from above.
				if (bIsElevatedTile(wOTileNo))
				{
					bNotAnObstacle = true;
					goto CheckFLayer;
				} else {
					CueEvents.Add(CID_BumpedLockedDoor, new CCoord(wOldX + dx, wOldY + dy), true);
					CueEvents.Add(CID_HitObstacle,
							new CMoveCoord(wOldX + dx,
									wOldY + dy, wMoveO), true);
				}
			break;
/*
				//Swordsman hit "master wall" and couldn't go through.
				//Replace command that bumped him into the wall with a WAIT
				//in the saved game command sequence so the saved game remains
				//consistent when replayed later at a point where the
				//player can go through this wall type.
				if (!this->Commands.IsFrozen())
				{
					this->Commands.RemoveLast();
					this->Commands.Add(CMD_WAIT);
				}
				CueEvents.Add(CID_HitObstacle,
						new CMoveCoord(wOldX + dx,
								wOldY + dy, wMoveO), true);
			break;
*/
			default:
				goto CheckFLayer;
			break;
		}
		//If here, that means a special o-layer obstacle was encountered and handled.
		//Player still can't move, however.
		dx = dy = 0;
		goto MakeMove;

CheckFLayer:
		//Check for special f/t-layer handling after checking o-layer.
		if (bIsArrowObstacle(wNewFTile, wMoveO) ||
				room.DoesSquarePreventDiagonal(wOldX, wOldY, dx, dy)) //can't go against arrows, etc
		{
			CueEvents.Add(CID_HitObstacle,
					new CMoveCoord(wOldX + dx,
							wOldY + dy, wMoveO), true);
			dx = dy = 0;
			goto MakeMove;
		}

		switch (wNewTTile)
		{
			case T_OBSTACLE:
			case T_BOMB:
			case T_ORB:
			case T_LIGHT:
			case T_TAR: case T_MUD: case T_GEL:
					CueEvents.Add(CID_HitObstacle,
							new CMoveCoord(wOldX + dx,
									wOldY + dy, wMoveO), true);
			break;

			case T_MIRROR:
			{
				//Player ran into mirror.  Push if possible.
				const UINT destX = wOldX + dx, destY = wOldY + dy;
				if (room.CanPlayerMoveOnThisElement(p.wAppearance,
						room.GetOSquare(destX, destY),
						bIsElevatedTile(room.GetOSquare(wOldX, wOldY))) &&
							room.CanPushTo(destX, destY, destX + dx, destY + dy))
				{
					//Push, if monster layer doesn't have an obstacle too.
					bPushingMirror = bNotAnObstacle = true;
					goto CheckMonsterLayer;
				}
				//Can't push.
				CueEvents.Add(CID_HitObstacle,
						new CMoveCoord(destX, destY, wMoveO), true);
			}
			break;
			case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
				CueEvents.Add(CID_Scared);
			break;
			default:
				goto CheckMonsterLayer;	//no special obstacle handling on t-layer
			break;
		}
		//If here, that means a special t-layer obstacle was encountered and handled.
		//Player still can't move, however.
		dx = dy = 0;
		goto MakeMove;

CheckMonsterLayer:
		//Check for special monster handling after checking o- and t-layers.
		if (!pMonster || p.IsInvisible())
		{
			//Every special obstacle check should have been performed by now.

			//If a seeming obstacle could actually be moved on, do it now.
			//(e.g. walking on top of a platform or pushing a mirror)
			if (bNotAnObstacle)
//OBSOLETE:
//can't move onto a sword even if the obstacle in the way is moveable/avoidable
//					&& !room.IsMonsterSwordAt(wOldX + dx, wOldY + dy))
			{
				//Can move platform now.
				if (bMovingPlatform)
					room.MovePlatform(wOldX, wOldY, nFirstO);

				//If mirror is being pushed, can do it now.
				if (bPushingMirror)
					room.PushObject(wOldX + dx, wOldY + dy,
							wOldX + dx*2, wOldY + dy*2, CueEvents);

				if (bJumping)
					CueEvents.Add(CID_Jump);

				if (bSwimming)
					CueEvents.Add(CID_Swim, new CCoord(wOldX + dx, wOldY + dy), true);

				goto MakeMove;
			}
		}

		//If here, that means obstacle is handled in normal way.
		CueEvents.Add(CID_HitObstacle,
				new CMoveCoord(wOldX + dx,
						wOldY + dy, wMoveO), true);
		dx = dy = 0;
	}

MakeMove:
	//Before player moves, remember important tile contents and conditions.
	const bool bCanDropTrapdoors = p.HasSword();
	const bool bReadyToDropTrapdoor = bIsTrapdoor(wOTileNo) && bCanDropTrapdoors;

	//Set player to new location.
	//If the player hasn't moved yet, this must be called even on a 0-delta
	//  movement in order to update the player's prev coords.
	if (!bEnteredTunnel && !bMoved)
		bMoved = SetPlayer(wOldX + dx, wOldY + dy);
	const UINT wNewOSquare = pRoom->GetOSquare(p.wX, p.wY);

	//These exclude the wait move executed on entering a room.
	const bool bWasOnSameScroll = (wTTileNo==T_SCROLL) && !bMoved && this->wTurnNo;
	const bool bStayedOnHotFloor = (wNewOSquare == T_HOT) && this->wTurnNo
			//flying player types don't get burned by hot tiles
			&& !p.IsFlying();

	//If obstacle was in the way, player's sword doesn't move.
	if (!bMoved && nCommand != CMD_C && nCommand != CMD_CC &&
			p.wSwordMovement != p.wO) //pushing forward is valid
		p.wSwordMovement = NO_ORIENTATION;

	const bool bSwordWasSheathed = !p.HasSword();
	//Swordless entities automatically face the way they're trying to move.
	if (bSwordWasSheathed && nFirstO != NO_ORIENTATION)
		p.SetOrientation(nFirstO);

	//Sword must be put away when on goo.
	if (!bIsStairs(wNewOSquare)) //don't update sword status when moving on stairs
		SetPlayerSwordSheathed();

	if (!p.HasSword())
		CueEvents.ClearEvent(CID_SwingSword); //don't fire the pending swing cue

	//Check for movement off of a trapdoor.
	if (bReadyToDropTrapdoor && bMoved)
		room.DestroyTrapdoor(wOldX, wOldY, CueEvents);

/*
	//Check for stepping on monster
	CMonster* pMonster = room.GetMonsterAtSquare(p.wX, p.wY);
	if (pMonster && (pMonster->wType == M_FEGUNDOASHES ||  //fegundo ashes
			p.CanStepOnMonsters()))       //player in monster-role attacked another monster
	{
		CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
		room.KillMonster(pMonster, CueEvents);
		TallyKill();
		pMonster->wO = p.wO; //splatter in this direction
		p.bIsTarget = true;  //monsters will attack a non-Beethro player now
	}
*/

	//Teleportation tunnels: handle move before sword hit.
	if (bEnteredTunnel)
	{
		CueEvents.Add(CID_Tunnel);
		TunnelMove(dx,dy);
	}

	ProcessPlayerMoveInteraction(dx, dy, CueEvents, bWasOnSameScroll, true);

	//Check for o-square things swordsman can step onto.
	bool bCannotLock = nCommand == CMD_LOCK;
	switch (wNewOSquare)
	{
		case T_STAIRS:       //Level exit.
		case T_STAIRS_UP:
			ProcessPlayer_HandleLeaveLevel(CueEvents);
		break;

		case T_DOOR_YO:
		case T_DOOR_GO:
		case T_DOOR_CO:
		case T_DOOR_MONEYO:
			if (nCommand == CMD_LOCK && !InCombat() && !this->dwCutScene)
			{
				bCannotLock = false;
				if (!LockDoor(CueEvents, p.wX, p.wY))
					CueEvents.Add(CID_BumpedLockedDoor, new CCoord(p.wX, p.wY), true);
			}
		break;

		case T_HOT:
			if (bStayedOnHotFloor)
			{
				const UINT damage = p.Damage(CueEvents, p.st.hotTileVal, CID_ExplosionKilledPlayer);
				if (damage) //only display effect if player is actually damaged
					CueEvents.Add(CID_PlayerBurned);
			}
		break;
		default: break;
	}
	if (bCannotLock)
		CueEvents.Add(CID_CantLockHere);
}

//***************************************************************************************
void CCurrentGame::ProcessPlayerMoveInteraction(int dx, int dy, CCueEvents& CueEvents,
	const bool bWasOnSameScroll, const bool bPlayerMove, const bool bPlayerTeleported)
{
	CSwordsman& p = *this->pPlayer; //shorthand
	CDbRoom& room = *this->pRoom;

	const bool bMoved = dx != 0 || dy != 0 || bPlayerTeleported;
//	const bool bSmitemaster = bIsSmitemaster(p.wAppearance);
//	const bool bCanGetItems = p.CanLightFuses();
	const UINT wOSquare = room.GetOSquare(p.wX, p.wY);
	const UINT wTSquare = room.GetTSquare(p.wX, p.wY);

	if (wOSquare == T_PRESSPLATE && bMoved && !p.IsFlying())
		room.ActivateOrb(p.wX, p.wY, CueEvents, OAT_PressurePlate);

	UINT wNewTSquare = room.GetTSquare(p.wX, p.wY);

	//Check for scroll events.
	if (!bWasOnSameScroll && wNewTSquare == T_SCROLL)
	{
		CDbMessageText* pScrollText = new CDbMessageText();
		const WSTRING wstr = GetScrollTextAt(p.wX, p.wY);
		*pScrollText = wstr.c_str();
		ASSERT((const WCHAR*)(*pScrollText)); //On assertion failure, room data is probably stored incorrectly.
		CueEvents.Add(CID_StepOnScroll, pScrollText, true);
	}

	//Check for t-layer items player can step onto and use.
	switch (wNewTSquare)
	{
	case T_ATK_UP:
	{
		const int atk = getItemAmount(wNewTSquare);
		incintValueWithBounds(p.st.ATK, atk);
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_ATK, atk), true);
		room.Plot(p.wX, p.wY, T_EMPTY);
		CueEvents.Add(CID_DrankPotion);
	}
	break;

	case T_DEF_UP:
	{
		const int def = getItemAmount(wNewTSquare);
		incintValueWithBounds(p.st.DEF, def);
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_DEF, def), true);
		room.Plot(p.wX, p.wY, T_EMPTY);
		CueEvents.Add(CID_DrankPotion);
	}
	break;

	case T_HEALTH_SM: case T_HEALTH_MED: case T_HEALTH_BIG:
	{
		const int heal = getItemAmount(wNewTSquare);
		if (heal < 0)
			p.DecHealth(CueEvents, -heal, CID_ExplosionKilledPlayer);
		else
		{
			incUINTValueWithBounds(p.st.HP, heal);
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_HEAL, heal), true);
		}
		room.Plot(p.wX, p.wY, T_EMPTY);
		CueEvents.Add(CID_DrankPotion);
	}
	break;

	/*
			case T_POTION_SP:  //Speed potion.
				{
					++p.st.speedPotions;
					room.Plot(p.wX, p.wY, T_EMPTY);
					CueEvents.Add(CID_DrankPotion);
				}
			break;
	*/

	case T_FUSE:
		//Light the fuse.
		room.LightFuse(CueEvents, p.wX, p.wY,
			this->wTurnNo > 0);  //start burning right away if lit on room entrance
		break;

	case T_TOKEN:
		//Activate token now, before the monsters move.
		if (bMoved || !this->wTurnNo)
			room.ActivateToken(CueEvents, p.wX, p.wY);
		break;

	case T_KEY:
	{
		PlayerStats& ps = p.st;
		const UINT tParam = room.GetTParam(p.wX, p.wY);
		switch (tParam)
		{
		case YellowKey: incUINTValueWithBounds(ps.yellowKeys, 1); break;
		case GreenKey: incUINTValueWithBounds(ps.greenKeys, 1); break;
		case BlueKey: incUINTValueWithBounds(ps.blueKeys, 1); break;
		case SkeletonKey: incUINTValueWithBounds(ps.skeletonKeys, 1); break;
		default: break;
		}
		room.Plot(p.wX, p.wY, T_EMPTY);
		CueEvents.Add(CID_ReceivedKey, new CAttachableWrapper<BYTE>(tParam), true);
	}
	break;
	case T_SWORD:
	{
		if (!bMoved && this->wTurnNo > 0)
			break; //don't keep trading equipment when standing on it

		//Acquired/traded a sword.
		const UINT oldEquipment = p.st.sword;
		const UINT newEquipment = room.GetTParam(p.wX, p.wY);
		TradeWeapon(CueEvents, newEquipment);
		//Place a weapon slot where this weapon was picked up and nothing was replaced.
		if (oldEquipment == NoSword && p.st.sword != NoSword)
		{
			ASSERT(room.GetTSquare(p.wX, p.wY) == T_EMPTY);
			room.Plot(p.wX, p.wY, T_SWORD);
			room.SetTParam(p.wX, p.wY, WeaponSlot);
		}
	}
	break;
	case T_SHIELD:
	{
		if (!bMoved && this->wTurnNo > 0)
			break; //don't keep trading equipment when standing on it

		//Acquired/traded a shield.
		const UINT oldEquipment = p.st.shield;
		const UINT newEquipment = room.GetTParam(p.wX, p.wY);
		TradeArmor(CueEvents, newEquipment);
		//Place an armor slot where this armor was picked up and nothing was replaced.
		if (oldEquipment == NoShield && p.st.shield != NoShield)
		{
			ASSERT(room.GetTSquare(p.wX, p.wY) == T_EMPTY);
			room.Plot(p.wX, p.wY, T_SHIELD);
			room.SetTParam(p.wX, p.wY, ArmorSlot);
		}
	}
	break;
	case T_ACCESSORY:
	{
		if (!bMoved && this->wTurnNo > 0)
			break; //don't keep trading equipment when standing on it

		//Acquired/traded an accessory.
		const UINT oldEquipment = p.st.accessory;
		const UINT newEquipment = room.GetTParam(p.wX, p.wY);
		TradeAccessory(CueEvents, newEquipment);
		//Place an accessory slot where this accessory was picked up and nothing was replaced.
		if (oldEquipment == NoAccessory && p.st.accessory != NoAccessory)
		{
			ASSERT(room.GetTSquare(p.wX, p.wY) == T_EMPTY);
			room.Plot(p.wX, p.wY, T_ACCESSORY);
			room.SetTParam(p.wX, p.wY, AccessorySlot);
		}
	}
	break;

	case T_MAP:
	{
		//Level map.
		//Mark all non-secret rooms in level on map.
		CIDSet roomsInLevel = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
		roomsInLevel -= GetExploredRooms(true); //ignore rooms already marked
		roomsInLevel -= room.dwRoomID;  //ignore current room
		for (CIDSet::const_iterator roomIter = roomsInLevel.begin();
			roomIter != roomsInLevel.end(); ++roomIter)
		{
			const UINT roomID = *roomIter;
			if (!CDbRoom::IsSecret(roomID))
			{
				ASSERT(!getExploredRoom(roomID));
				AddRoomToMap(roomID);
			}
		}

		room.Plot(p.wX, p.wY, T_EMPTY);
		CueEvents.Add(CID_LevelMap);
	}
	break;

	default:        //normal step (footfalls)
		if (bMoved)
			CueEvents.Add(CID_Step);
		break;
	}

	//Check for things that sword could hit.
	if (p.HasSword())
		ProcessSwordHit(p.GetSwordX(), p.GetSwordY(), CueEvents);
}

//***************************************************************************************
void CCurrentGame::ProcessPlayer_HandleLeaveLevel(
//Called when the player walked onto stairs or a scripted event was triggered
//warping to a new level entrance.
//
//Ending state:      The game will be inactive.  A cue event of either CID_WinGame or
//             CID_ExitLevelPending will have been added.
//Side effects:      Demos may be saved.
//
//Params:
	CCueEvents &CueEvents,  //(out)  Events added to it.
	const UINT wEntrance,   //if set, go to this level entrance [default=EXIT_LOOKUP]
	const bool bSkipEntranceDisplay) //[default=false]
{
	//If exiting level not from stairs, the destination ID should be specified.
	ASSERT(bIsStairs(this->pRoom->GetOSquare(this->pPlayer->wX, this->pPlayer->wY)) ||
			wEntrance != (UINT)EXIT_LOOKUP);

	//If a critical character died on exit move, the exit doesn't count.
	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
		return;

/*
	//Write a demo record if recording.
	if (this->bIsDemoRecording)
	{
		const UINT dwDemoID = WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

		//Set demo recording info to begin in a new room.
		this->DemoRecInfo.wBeginTurnNo = 0;
		this->DemoRecInfo.dwPrevDemoID = dwDemoID;
	}
*/

/*
	//Do things for a conquered room.
	const bool bConquered = WasRoomConqueredOnThisVisit();
	if (bConquered)
	{
		//Save a conquer demo if I need to.
		if ( (this->dwAutoSaveOptions & ASO_CONQUERDEMO)==ASO_CONQUERDEMO )
			WriteCurrentRoomConquerDemo();

		//If player exited the room after killing monsters in the room, then
		//add a cue event and set room's official status to conquered.  For rooms
		//that have no monsters, this would already have happened on the player's
		//first step into the room.
		if (!IsCurrentRoomConquered())
		{
			CueEvents.Add(CID_ConquerRoom);
			SetCurrentRoomConquered();
		}
	}
*/

	//If exiting via stairs, check for a defined exit destination for this room region.
	UINT dwEntranceID=wEntrance;
	if (wEntrance == (UINT)EXIT_LOOKUP)
	{
		for (UINT i=0; i<this->pRoom->Exits.size(); ++i)
		{
			CExitData *pStairs = this->pRoom->Exits[i];
			if (this->pPlayer->wX >= pStairs->wLeft && this->pPlayer->wX <= pStairs->wRight &&
					this->pPlayer->wY >= pStairs->wTop && this->pPlayer->wY <= pStairs->wBottom) 
			{
				//Robustness measure: verify this entrance record actually exists.
				if (pStairs->dwEntranceID && pStairs->dwEntranceID != UINT(EXIT_PRIOR_LOCATION) &&
						!this->pHold->GetEntrance(pStairs->dwEntranceID))
				{
					//Exit has dangling entrance ID -- ignore it and keep looking.
					continue;
				}
				dwEntranceID = pStairs->dwEntranceID;
				break;
			}
		}
		if (dwEntranceID == (UINT)EXIT_LOOKUP) //no exit destination defined by stairs
			dwEntranceID = EXIT_ENDHOLD;  //so end the hold
	}

	//Returning to a prior warp location is only possible when one has been defined.
	if (dwEntranceID == (UINT)EXIT_PRIOR_LOCATION)
	{
		if (!this->pPlayer->st.priorRoomID)
		{
			//Nowhere to go.
			if (wEntrance != (UINT)EXIT_LOOKUP)
				return;

			dwEntranceID = EXIT_ENDHOLD; //stairs force ending the hold
		}
		else if (!g_pTheDB->Rooms.Exists(this->pPlayer->st.priorRoomID))
		{
			//This condition could happen when the destination room was deleted
			//in the editor since the time this game was last played/saved.
			//
			//We must be robust to it.
			if (wEntrance != (UINT)EXIT_LOOKUP)
				return; //destination not valid -- don't travel anywhere

			dwEntranceID = EXIT_ENDHOLD; //stairs force ending the hold
		} else {
			//Can't update prior location until we've loaded it in LoadFromLevelEntrance
			ASSERT(!this->pPendingPriorLocation);
			CSwordsman& p = *this->pPlayer;
			if (wEntrance == UINT(EXIT_LOOKUP)) {
				this->pPendingPriorLocation = new CMoveCoordEx(
						p.wPrevX, p.wPrevY, p.wPrevO, this->pRoom->dwRoomID);
			} else {
				this->pPendingPriorLocation = new CMoveCoordEx(
						p.wX, p.wY, p.wO, this->pRoom->dwRoomID);
			}
		}
	} else {
		//Warping to a level entrance.  Store the room+coords where coming from.
		CSwordsman& p = *this->pPlayer;
		PlayerStats& ps = p.st;
		ps.priorRoomID = this->pRoom->dwRoomID;
		if (wEntrance == UINT(EXIT_LOOKUP)) {
			ps.priorX = p.wPrevX;
			ps.priorY = p.wPrevY;
			ps.priorO = p.wPrevO;
		} else {
			ps.priorX = p.wX;
			ps.priorY = p.wY;
			ps.priorO = p.wO;
		}
	}

	//If no exits, this signifies the end of the hold, i.e.,
	//If there are no levels to go to next, then the game has been won.
	CEntranceData *pEntrance = this->pHold->GetEntrance(dwEntranceID);
	if (pEntrance)
	{
		//Send back level to go to next.
		//ProcessCommand() caller must invoke LoadFromLevelEntrance
		//to continue play at the appropriate location.
		CueEvents.Add(CID_ExitLevelPending,
				new CCoord(dwEntranceID, bSkipEntranceDisplay), true);
	} else {
		//Return to the location where the prior Level Entrance script command
		//was invoked.
		if (dwEntranceID == (UINT)EXIT_PRIOR_LOCATION)
		{
			//ProcessCommand() caller must invoke LoadFromLevelEntrance
			//to continue play at the appropriate location.
			ASSERT(this->pPlayer->st.priorRoomID);
			CueEvents.Add(CID_ExitLevelPending, new CCoord(EXIT_PRIOR_LOCATION, true), true);
		} else {
			//End the hold.

			//Stop any multi-room demo sequence being recorded.
//			this->bIsDemoRecording = false;

			CueEvents.Add(CID_WinGame);
			if (!this->Commands.IsFrozen())
			{
				++this->pPlayer->st.totalMoves; //Add move before saving.  Another move will be tallied after saving, but it is ignored
				SaveToEndHold();
			}
		}
	}

	ExitCurrentRoom();

	//A load is needed before more commands can be processed.
	this->bIsGameActive = false;

	UpdatePrevCoords();
}

//***************************************************************************************
bool CCurrentGame::ProcessPlayer_HandleLeaveRoom(
//This is a hunk of code yanked out of ProcessPlayer() for readability.
//
//Beginning state:   Player is standing on the edge of a room and user has 
//             issued a command to move out of the room.
//Ending state:      If successful, player will either be positioned in the new room,
//             or the game will be inactive and ready for a new room load.
//Side effects:      Demos may be saved.  
//             Cue events may be added.  
//             Current room may be set to conquered.
//             Game may become inactive if this call is during demo playback.
//
//Params:
	const UINT wMoveO,      //(in)   Direction of movement that leaves the room.
	CCueEvents &CueEvents)  //(out)  Events added to it.
//
//Returns:
//True if player left the room successfully, false if player's departure was
//unsuccessful (ProcessPlayer() should keep player in current room).
{
	//Is there an adjoining room in the direction of exit?
	const UINT wExitDirection = GetRoomExitDirection(wMoveO);
	UINT dwNewSX, dwNewSY;
	CDbRoom *pNewRoom = NULL;
	if (!LoadAdjoiningRoom(wExitDirection, dwNewSX, dwNewSY, pNewRoom))
	{
		CueEvents.Add(CID_NoAdjoiningRoom);
		delete pNewRoom;
		return false;
	}

	//Can the player move onto the proposed entrance tile?
	if (!pNewRoom->CanSetSwordsman(dwNewSX, dwNewSY))
	{
		CueEvents.Add(CID_ExitBlockedOnOtherSide);
		delete pNewRoom;
		return false;
	}

/*
	//Write a demo record if recording.
	if (this->bIsDemoRecording)
	{
		const UINT dwDemoID = WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

		//Set demo recording info to begin in a new room.
		this->DemoRecInfo.wBeginTurnNo = 0;
		this->DemoRecInfo.dwPrevDemoID = dwDemoID;
	}
*/

	//If commands are frozen (demo playback), then don't load a new room right now.
	//Instead, add exit pending event and return.
	//However, if playback is simply being validated, we don't need to wait for a load.
	if (this->Commands.IsFrozen() && !IsValidatingPlayback())
	{
		CueEvents.Add(CID_ExitRoomPending, new CAttachableWrapper<UINT>(wExitDirection), true);
		this->bIsGameActive = false;  //A load is needed before more commands 
		                              //can be processed.
		delete pNewRoom;
		UpdatePrevCoords();     //fix characters at their current location
		return true;
	}

	ExitCurrentRoom();

	CDbSavedGame::SetMonsterListAtRoomStart();

	//Leave the room.

	//LoadAdjoiningRoom called RetrieveExploredRoomData,
	//so it doesn't need to be called again on this room object during setup.
	LoadNewRoomForExit(dwNewSX, dwNewSY, pNewRoom, CueEvents);  //retain pNewRoom

	CueEvents.Add(CID_ExitRoom, new CAttachableWrapper<UINT>(wExitDirection), true);
	return true;
}

//***************************************************************************************
bool CCurrentGame::HasUnansweredQuestion(const CMonster* pMonster) const
{
	for (list<CMonsterMessage>::const_iterator it=this->UnansweredQuestions.begin();
			it!=this->UnansweredQuestions.end(); ++it)
	{
		if (it->pSender == pMonster)
			return true;
	}

	return false;
}

//***************************************************************************************
void CCurrentGame::ProcessUnansweredQuestions(
//Processes unanswered questions.  Game is in a state where it is waiting for player
//to answer one of more questions posed by monsters.  If multiple questions have
//been asked, then game will take an answer for the first question asked, then the second, 
//etc.
//
//Params:
	const int nCommand,              //(in)      CMD_YES or CMD_NO, or answer #.
	list<CMonsterMessage> &UnansweredQuestions,  //(in/out)  Receives list of unanswered
												//       questions.  If command is
												//       a valid answer to the first,
												//       the the first question will
												//       be removed from list.
	CCueEvents &CueEvents)                 //(out)     Adds cue events as appropriate.
{
	list<CMonsterMessage>::const_iterator iQuestion = UnansweredQuestions.begin();
	switch (iQuestion->eType)
	{
		case MMT_YESNO:
			if (nCommand != CMD_YES && nCommand != CMD_NO)
			{
				//This command should not have been sent.  Possible causes:
				//- the front end has been implemented incorrectly and sent a bad command.
				//- a demo has been corrupted or misrecorded.
				ASSERT(!"Bad command.");
				this->bIsGameActive = false;
				return;
			}

			//Send answer to question's sender.
			if (iQuestion->pSender)
				iQuestion->pSender->OnAnswer(nCommand, CueEvents);
		break;
		case MMT_MENU:
			//Send answer to question's sender.
			if (iQuestion->pSender)
				iQuestion->pSender->OnAnswer(nCommand, CueEvents);
		break;
		default:
			ASSERT(!"Unexpected question type.");
		break;
	}

	//Remove the question that was just answered from the list.
	UnansweredQuestions.pop_front();
}

//*****************************************************************************
void CCurrentGame::PackData(CDbPackedVars& stats)
//Pack various game data into a packed vars blob.
{
	this->pPlayer->st.Pack(stats);

	stats.SetVar(shieldOffStr, this->pPlayer->bShieldOff);
	stats.SetVar(accessoryOffStr, this->pPlayer->bAccessoryOff);
	stats.SetVar(commandOffStr, this->pPlayer->bCommandOff);

	//Pack music.
	stats.SetVar(musicEnumStr, this->music.musicEnum);
	stats.SetVar(musicIDStr, this->music.musicID);
	stats.SetVar(playMusicIDStr, this->music.bPlayMusicID);
	stats.SetVar(songMoodStr, this->music.songMood.c_str());
}

//*****************************************************************************
void CCurrentGame::UnpackData(CDbPackedVars& stats)
//Unpack various game data from a packed vars blob.
{
	this->pPlayer->st.Unpack(stats);

	this->pPlayer->bShieldOff = stats.GetVar(shieldOffStr, this->pPlayer->bShieldOff);
	this->pPlayer->bAccessoryOff = stats.GetVar(accessoryOffStr, this->pPlayer->bAccessoryOff);
	this->pPlayer->bCommandOff = stats.GetVar(commandOffStr, this->pPlayer->bCommandOff);

	//Unpack music.
	this->music.clear();
	this->music.musicEnum = stats.GetVar(musicEnumStr, this->music.musicEnum);
	this->music.musicID = stats.GetVar(musicIDStr, this->music.musicID);
	this->music.bPlayMusicID = stats.GetVar(playMusicIDStr, this->music.bPlayMusicID);
	this->music.songMood = stats.GetVar(songMoodStr, this->music.songMood.c_str());
}

//*****************************************************************************
void CCurrentGame::RetrieveExploredRoomData(CDbRoom& room)
//Loads the state of mutable room elements of the room being entered from the prior visit.
{
	ExploredRoom *pExpRoom = getExploredRoom(room.dwRoomID);
	if (!pExpRoom)
		return;
	
	room.mapMarker = pExpRoom->mapMarker;

	pExpRoom->bSave = true; //previewed room now included in save data

	if (pExpRoom->bMapOnly)
	{
		//Room is on the map but hasn't been explored previously.
		//Now the player is arriving here for first time, so record the room as explored.
		pExpRoom->bMapOnly = false;
		return;
	}

	room.SetMembersFromExploredRoomData(pExpRoom);
}

//*****************************************************************************
void CCurrentGame::SaveExploredRoomData(CDbRoom& room)
//Adds or updates an ExploredRoom record to keep track of what is in the room being left.
{
	//Has this room already been explored?
	ExploredRoom *pExpRoom = getExploredRoom(room.dwRoomID);
	if (!pExpRoom)
	{
		pExpRoom = new ExploredRoom();
		this->ExploredRooms.push_back(pExpRoom);
		pExpRoom->roomID = room.dwRoomID;
	} else {
		//Delete old data.
		pExpRoom->litFuses.clear();
		pExpRoom->platformDeltas.Clear();
//		pExpRoom->orbTypes.clear();
		pExpRoom->deleteMonsters();
		pExpRoom->pMonsterList = NULL;
	}

	pExpRoom->bMapOnly = false;
	pExpRoom->mapMarker = room.mapMarker;

	//Save room state.
	c4_Bytes *pBytes = room.PackSquares();
	pExpRoom->SquaresBytes = c4_Bytes(pBytes->Contents(), pBytes->Size(), true);
	delete pBytes;

	for (CCoordSet::const_iterator fuse = room.LitFuses.begin();
			fuse != room.LitFuses.end(); ++fuse)
		pExpRoom->litFuses += room.ARRAYINDEX(fuse->wX, fuse->wY);

	UINT count;
	for (count=0; count<room.platforms.size(); ++count)
	{
		const CPlatform& platform = *(room.platforms[count]);
		pExpRoom->platformDeltas.Push(platform.xOffset, platform.yOffset);
	}

	/*
	for (count=0; count<room.orbs.size(); ++count)
		pExpRoom->orbTypes.push_back(room.orbs[count]->eType);
	*/

	//Save monsters.
	CMonster *pLastNewMonster = NULL;
	for (CMonster *pMonster = room.pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		//Save monster state info where needed.
		if (pMonster->wType == M_CHARACTER)
		{
			//Restart scripts that are flagged to do so.
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsRestartScriptOnRoomEntrance())
				pCharacter->RestartScript();

			//Save current state of NPC, but the NPCs script itself doesn't need to be saved,
			//and should not be included to save space.
			pCharacter->SetExtraVarsFromMembersWithoutScript(pCharacter->ExtraVars);
		}

		//Make copy of monster in its current state.
		CMonster *pNew = pMonster->Clone();
		//Copy monster pieces.
		pNew->Pieces.clear();  //make new copies of pieces
		for (list<CMonsterPiece*>::iterator piece = pMonster->Pieces.begin();
				piece != pMonster->Pieces.end(); ++piece)
		{
			CMonsterPiece& oldPiece = *(*piece);
			CMonsterPiece *pNewPiece = new CMonsterPiece(pNew,
					oldPiece.wTileNo, oldPiece.wX, oldPiece.wY);
			pNew->Pieces.push_back(pNewPiece);
		}

		pNew->ResetCurrentGame();

		//Link up.
		pNew->pNext = NULL;
		pNew->pPrevious = pLastNewMonster;
		if (!pExpRoom->pMonsterList)
			pExpRoom->pMonsterList = pNew;
		else
		{
			ASSERT(pLastNewMonster);
			pLastNewMonster->pNext = pNew;
		}
		pLastNewMonster = pNew;
	}

	//All other data should be non-mutable or reconstructible on room init.
}

//***************************************************************************************
void CCurrentGame::SetMembers(const CCurrentGame &Src)
//Performs deep copy.
{
	CDbSavedGame::operator=(Src);

	//Resync global monsters to this game object
	CMonster *pMonster;
	for (pMonster = this->pMonsterList; pMonster != NULL; pMonster = pMonster->pNext) {
		pMonster->ResetCurrentGame();
		pMonster->SetCurrentGame(this);
	}
	for (pMonster = this->pMonsterListAtRoomStart; pMonster != NULL; pMonster = pMonster->pNext) {
		pMonster->ResetCurrentGame();
		pMonster->SetCurrentGame(this);
	}

	ASSERT(Src.pHold);
	if (this->pHold)
		delete this->pHold;
	this->pHold = new CDbHold(*Src.pHold);
	ASSERT(Src.pLevel);
	if (this->pLevel)
		delete this->pLevel;
	this->pLevel = new CDbLevel(*Src.pLevel);
	ASSERT(Src.pRoom);
	if (this->pRoom)
		delete this->pRoom;
	this->pRoom = new CDbRoom(*Src.pRoom);
	this->wTurnNo = Src.wTurnNo; //set before calling SetCurrentGame
	this->pRoom->SetCurrentGame(this);

	this->pEntrance = Src.pEntrance ? this->pHold->GetEntrance(Src.pEntrance->dwEntranceID) : NULL;

	this->pPlayer = new CSwordsman(*Src.pPlayer);

	this->bIsGameActive = Src.bIsGameActive;
	this->wPlayerTurn = Src.wPlayerTurn;
	this->wSpawnCycleCount = Src.wSpawnCycleCount;
	this->bHalfTurn = Src.bHalfTurn;
/*
	this->bIsDemoRecording = Src.bIsDemoRecording;
	this->wMonsterKills = Src.wMonsterKills;
	this->bBrainSensesSwordsman = Src.bBrainSensesSwordsman;
	this->wLastCheckpointX = Src.wLastCheckpointX;
	this->wLastCheckpointY = Src.wLastCheckpointY;
	this->checkpointTurns = Src.checkpointTurns;
	this->bHoldMastered = Src.bHoldMastered;
	this->bWaitedOnHotFloorLastTurn = Src.bWaitedOnHotFloorLastTurn;
*/
	this->dwStartTime = Src.dwStartTime;
	this->dwCutScene = Src.dwCutScene;
	this->bContinueCutScene = Src.bContinueCutScene;
	this->statsAtRoomStart = Src.statsAtRoomStart;
	this->roomsExploredAtRoomStart = Src.roomsExploredAtRoomStart;
	this->roomsMappedAtRoomStart = Src.roomsMappedAtRoomStart;
	this->ambientSounds = Src.ambientSounds;

	//Speech log.
	vector<CCharacterCommand*>::const_iterator iter;
	for (iter = this->roomSpeech.begin();	iter != this->roomSpeech.end(); ++iter)
		delete *iter;
	this->roomSpeech.clear();
	for (iter = Src.roomSpeech.begin();	iter != Src.roomSpeech.end(); ++iter)
	{
		CCharacterCommand& c = *(*iter);
		this->roomSpeech.push_back(new CCharacterCommand(c));
	}
	this->music = Src.music;

	//Combat.
	this->bQuickCombat = Src.bQuickCombat;
	this->pCombat = NULL;
	if (Src.pCombat)
	{
		this->pCombat = new CCombat(*Src.pCombat);
		this->pCombat->pGame = this;
		this->pCombat->pMonster = this->pRoom->GetMonsterAtSquare(
				Src.pCombat->pMonster->wX, Src.pCombat->pMonster->wY);
	}
	if (Src.pBlockedSwordHit)
		this->pBlockedSwordHit = this->pRoom->GetMonsterAtSquare(
				Src.pBlockedSwordHit->wX, Src.pBlockedSwordHit->wY);
	else this->pBlockedSwordHit = NULL;

	this->pendingTokenActivations = Src.pendingTokenActivations;
	this->simulSwordHits = Src.simulSwordHits;
	//this->possibleTarStabs = Src.possibleTarStabs; //only valid this current turn
	//this->changingInventoryTypes = Src.changingInventoryTypes; //only valid this current turn
	//this->pPendingPriorLocation = Src.pPendingPriorLocation? new CMoveCoordEx(*Src.pPendingPriorLocation) : NULL; //only valid this current turn

//	this->DemoRecInfo = Src.DemoRecInfo;

	this->UnansweredQuestions.clear();
	for (list<CMonsterMessage>::const_iterator uq = Src.UnansweredQuestions.begin();
			uq != Src.UnansweredQuestions.end(); ++uq)
	{
		CMonsterMessage mm(*uq);
		if (mm.pSender)
		{
			//Pointer to the source monster must be updated
			CMonster *pMonster;
			if (mm.pSender->wType == M_CHARACTER)
			{
				//Find NPC with same ID.
				CCharacter *pSrcCharacter = DYN_CAST(CCharacter*, CMonster*, mm.pSender);
				const UINT scriptID = pSrcCharacter->dwScriptID;
				mm.pSender = NULL;
				for (pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
				{
					if (pMonster->wType == M_CHARACTER)
					{
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
						if (pCharacter->dwScriptID == scriptID) //found a match
						{
							mm.pSender = pMonster;
							break;
						}
					}
				}
				ASSERT(mm.pSender);
			} else {
				//Otherwise, the only way to match them up is by position in the monster list.
				UINT wIndex=0;
				for (pMonster = Src.pRoom->pFirstMonster; pMonster != NULL;	pMonster = pMonster->pNext, ++wIndex)
					if (mm.pSender == pMonster)
						break;
				mm.pSender = NULL;
				if (pMonster)
				{
					for (pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext, --wIndex)
						if (!wIndex)
						{
							mm.pSender = pMonster;
							break;
						}
					ASSERT(mm.pSender); //should have been assigned
				}
				//else if NULL, pSender was not found in the original monster list
			}
		}
		this->UnansweredQuestions.push_back(mm);
		
	}
	this->bIsNewRoom = Src.bIsNewRoom;
	this->bExecuteNoMoveCommands = Src.bExecuteNoMoveCommands;

//	this->dwAutoSaveOptions = Src.dwAutoSaveOptions;
	this->CompletedScriptsPending = Src.CompletedScriptsPending;
	this->bNoSaves = Src.bNoSaves;
	this->bValidatingPlayback = Src.bValidatingPlayback;

/*
	this->dwComputationTime = Src.dwComputationTime;
	this->dwComputationTimePerSnapshot = Src.dwComputationTimePerSnapshot;
*/
}

//***************************************************************************************
void CCurrentGame::SetMembersAfterRoomLoad(
//Sets members of current game object that should be changed whenever
//a new room is loaded.
//
//Params:
	CCueEvents &CueEvents,  //(out)  Cue events generated by swordsman's first step 
							//    into the room.
	const bool bResetCommands) //(in) [default = true]
{
	ASSERT(this->pRoom);

	this->pPlayer->ResetRoomStats();

	this->bIsGameActive = true;
	this->wSpawnCycleCount = this->wPlayerTurn = this->wTurnNo = 0;
	this->bHalfTurn = false;
//	this->bWaitedOnHotFloorLastTurn = false;

	this->dwCutScene = 0;

	this->pRoom->BurnFuseEvents(CueEvents);

	CDbSavedGame::setMonstersCurrentGame(this);
	this->pRoom->SetCurrentGame(this);
	this->dwRoomID = this->pRoom->dwRoomID;

	this->dwPlayerID = g_pTheDB->GetPlayerID();

/*
	//Update demo recording information.
	this->DemoRecInfo.wBeginTurnNo = 0;
*/

/*
	//Level is considered already complete (skipping CID_CompleteLevel) only
	//if the level isn't being entered for the first time.
	const bool bWasLevelComplete = IsCurrentLevelComplete() && !CDbSavedGame::ExploredRooms.empty();
*/

	//Has this room been previously explored?
	this->bIsNewRoom = !IsCurrentRoomExplored(false);
/*
	if (this->bIsNewRoom)
		SetCurrentRoomExplored();
	const bool bWasRoomConquered = IsCurrentRoomConquered();
*/

	//Before removing anything on player entrance, init these stats.
//	UINT wMonsterCountAtStart = this->pRoom->wMonsterCount;
//	this->pRoom->bGreenDoorsOpened = false;

	if (pRoom->IsDisarmTokenActive())
		ToggleSwordDisarm();

	if (this->pPlayer->IsInRoom())
	{
		//Remove a crumbly wall, bomb or monster underneath the swordsman if it exists.
		if (bIsCrumblyWall(this->pRoom->GetOSquare(this->pPlayer->wX, this->pPlayer->wY))
				&& this->pPlayer->wAppearance != M_SEEP) //seep-player can remain on the wall
			this->pRoom->DestroyCrumblyWall(this->pPlayer->wX, this->pPlayer->wY, CueEvents);
		const UINT tTile = this->pRoom->GetTSquare(this->pPlayer->wX, this->pPlayer->wY);
		if (tTile == T_BOMB || tTile == T_MIRROR)
			this->pRoom->Plot(this->pPlayer->wX, this->pPlayer->wY, T_EMPTY);
//		CCueEvents Ignored; //don't receive cues if this monster is not really there

		//Remove any monster here.
		this->pRoom->KillMonsterAtSquare(this->pPlayer->wX, this->pPlayer->wY, CueEvents);
	}

/*
	//See if room is already conquered.
	if (bWasRoomConquered)
	{
		ToggleGreenDoors();
		this->pRoom->ClearMonsters(true);
		wMonsterCountAtStart = 0;
	}
	//Not conquered, but if no monsters in it then add to conquered list immediately.
	else if (!wMonsterCountAtStart)
	{
		CueEvents.Add(CID_ConquerRoom);
		ToggleGreenDoors();
		this->pRoom->ClearMonsters(true); //remove monsters that only show in unconquered rooms
		SetCurrentRoomConquered();

		//When restoring game to the the first-time entry of a clean room,
		//ensure this flag is set to allow resaving victory demo on room exit.
		this->bIsNewRoom = true;
	}
	else {
*/
	//Clear first turn status on new room monsters.
	this->pRoom->ResetMonsterFirstTurnFlags();
/*
	}

	this->pRoom->SetHalphSlayerEntrance();
*/

// Shouldn't have to be done since persistent room state is maintained.
//	this->pRoom->RemoveFinishedCharacters();

	//Clear dead monsters previously stored in room.  They were kept in
	//memory to keep private data pointers valid.
	this->pRoom->ClearDeadMonsters();

	//Remove any monster messages left unprocessed.
	this->UnansweredQuestions.clear();

	this->pRoom->KillSeepOutsideWall(CueEvents); //remove before any doors change

/*
	//Remove blue doors if level is complete.
	if (IsCurrentLevelComplete()) 
	{
		if (!bWasLevelComplete) CueEvents.Add(CID_CompleteLevel);
		this->pRoom->ToggleTiles(T_DOOR_C, T_DOOR_CO); //blue/exit door
	}
*/

	//Flag if secret room entered.
	if (this->bIsNewRoom && this->pRoom->bIsSecret)
		CueEvents.Add(CID_SecretRoomFound);

/*
	//Remove red doors if no trapdoors in the room.
	if (this->pRoom->wTrapDoorsLeft == 0)
		this->pRoom->ToggleTiles(T_DOOR_R, T_DOOR_RO); //red door
*/

	//When entering a new room, make sure all tarstuff is stable.
	if (this->bIsNewRoom)
	{
		if (this->pRoom->wTarLeft)
			this->pRoom->FixUnstableTar(CueEvents);
		//In this game, black gates don't toggle automatically if an empty room has no tar.
/*
		//If no tarstuff is in the room to begin with, then toggle black gates now.
		else
			this->pRoom->ToggleBlackGates(CueEvents);
*/
	}


	//Mark which pressure plates are depressed on entrance.
	this->pRoom->SetPressurePlatesState();

	//No combat should be occurring at this point.
	delete this->pCombat;
	this->pCombat = NULL;
	this->pBlockedSwordHit = NULL;

	//Process the swordsman's movement onto the first square.
	if (this->pPlayer->IsInRoom() && !this->pPlayer->bHasTeleported)
		ProcessPlayer(CMD_WAIT, CueEvents);

	//Init NPCs and sworded entities after initial room state checks and modifications are performed.
	this->bExecuteNoMoveCommands = true;
	ProcessScripts(CMD_WAIT, CueEvents, CDbSavedGame::pMonsterList);
	this->pRoom->PreprocessMonsters(CueEvents);
	this->bExecuteNoMoveCommands = false;

	ProcessSimultaneousSwordHits(CueEvents);  //destroy simultaneously-stabbed tar
	this->pRoom->KillSeepOutsideWall(CueEvents); //check again if doors have changed
	SetPlayerMood(CueEvents);
	this->pRoom->ResetMonsterFirstTurnFlags();
/*
	//Check whether swordsman's first step left room without monsters.
	if (!this->pRoom->wMonsterCount && wMonsterCountAtStart)
	{
		//if so, remove green door
		ToggleGreenDoors();
		//Don't mark room conquered until player exits the room.
	}
*/

	if (bResetCommands && !this->Commands.IsFrozen())
		this->Commands.Clear();

	//Player should always be visible if no cut scene is playing.
	if (!this->pPlayer->IsInRoom() && !this->dwCutScene)
		SetPlayerRole(defaultPlayerType()); //place player in room now as default type

/*
	//Setup PathMap to player for monsters that require it.
	this->pRoom->CreatePathMaps();
*/

	//Player should never die on entering room.
	ASSERT(!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied),CIDA_PlayerDied));

	//Characters can catch events occurring on room entrance.
	this->pRoom->CharactersCheckForCueEvents(CueEvents, this->pRoom->pFirstMonster);
	this->pRoom->CharactersCheckForCueEvents(CueEvents, CDbSavedGame::pMonsterList); //global scripts

	AddRoomsToPlayerTally();

	//Reset ambient sounds and speech.
	this->ambientSounds.clear();
	for (vector<CCharacterCommand*>::const_iterator iter = this->roomSpeech.begin();
			iter != this->roomSpeech.end(); ++iter)
		delete *iter;
	this->roomSpeech.clear();

	//Get sounds+speech queued on room entrance.
	AmbientSoundTracking(CueEvents);

	//Don't start any fights on room entrance.
	delete this->pCombat;
	this->pCombat = NULL;
	this->pBlockedSwordHit = NULL;
}

//*****************************************************************************
void CCurrentGame::SetPlayerMood(
//Determine player's mood, according to relative position of monsters.
//Scared overrides aggressive, aggressive overrides tired/normal mood.
//
//Params:
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
{
	CMonster *pMonster;
	UINT x, y;

	const UINT wX = this->pPlayer->wX, wY = this->pPlayer->wY;
/*
	//Scared?
	//(whether aggressive monster is behind Beethro)
	if (this->pPlayer->wAppearance == M_BEETHRO)
	{
		const UINT wO = this->pPlayer->wO;
		const int oX = nGetOX(wO);
		const int oY = nGetOY(wO);
		CCoordStack checkSquares;

		//Determine which squares to check.
		if (wX-oX < this->pRoom->wRoomCols) //avoid assertion
			checkSquares.Push(wX-oX,wY-oY);  //check directly behind swordsman
		//check behind on left/right sides
		if (oX && oY)  //swordsman is facing diagonally
		{
			checkSquares.Push(wX-oX,wY);
			checkSquares.Push(wX,wY-oY);
		} else { //swordsman is facing straight
			if (oX && wX-oX < this->pRoom->wRoomCols) //avoid assertion
			{
				checkSquares.Push(wX-oX,wY-1);
				checkSquares.Push(wX-oX,wY+1);
			}
			else if (wY-oY < this->pRoom->wRoomRows)  //avoid assertion
			{  //oY
				checkSquares.Push(wX-1,wY-oY);
				checkSquares.Push(wX+1,wY-oY);
			}
		}

		//Check squares for monster.
		while (checkSquares.GetSize()) {
			checkSquares.Pop(x,y);
			if (this->pRoom->IsValidColRow(x,y))
			{
				pMonster = this->pRoom->GetMonsterAtSquare(x,y);
				if (this->pRoom->MonsterHeadIsAt(x,y) && pMonster->IsAggressive())
				{
					CueEvents.Add(CID_SwordsmanAfraid);
					while (checkSquares.GetSize())   //remove remaining squares
						checkSquares.Pop(x,y);
					return;  //don't check for other moods
				}
			}
		}
	}
*/

	//Aggressive?
	//(whether sword is adjacent to a non-friendly monster)
	if (this->pPlayer->HasSword())
	{
		const UINT wSX = this->pPlayer->GetSwordX(), wSY = this->pPlayer->GetSwordY();   //shorthand
		for (x=wSX-1; x<=wSX+1; ++x)
			for (y=wSY-1; y<=wSY+1; ++y)
			{
				if (x==wSX && y==wSY) continue;
				if (this->pRoom->IsValidColRow(x,y))
				{
					pMonster = this->pRoom->GetMonsterAtSquare(x,y);
					if (pMonster && !pMonster->IsFriendly())
					{
						CueEvents.Add(CID_SwordsmanAggressive);
						return;  //don't check for other moods
					}
				}
			}
	}

	//Also aggressive when standing next to a monster.
	for (x=wX-1; x<=wX+1; ++x)
		for (y=wY-1; y<=wY+1; ++y)
		{
			if (x==wX && y==wY) continue;
			if (this->pRoom->IsValidColRow(x,y))
			{
				pMonster = this->pRoom->GetMonsterAtSquare(x,y);
				if (pMonster && !pMonster->IsFriendly())
				{
					CueEvents.Add(CID_SwordsmanAggressive);
					return;  //don't check for other moods
				}
			}
		}


	CueEvents.Add(CID_SwordsmanNormal);
}

//*****************************************************************************
void CCurrentGame::SetPlayerToRoomStart()
//Sets player-related members to start of room.
{
	SetPlayerRole(this->wStartRoomAppearance);
	this->pPlayer->bSwordOff = CDbSavedGame::bStartRoomSwordOff;
	this->pPlayer->SetOrientation(this->wStartRoomO);
	SetPlayer(this->wStartRoomX, this->wStartRoomY);
//	this->wLastCheckpointX = this->wLastCheckpointY = static_cast<UINT>(-1);
//	this->checkpointTurns.clear();
	this->CompletedScriptsPending.clear();
	this->stats = this->statsAtRoomStart;
	RemoveMappedRoomsNotIn(this->roomsExploredAtRoomStart, this->roomsMappedAtRoomStart);

	//Prepare vars for recording saved games.
	this->bIsGameActive = true;
	this->wTurnNo = 0;
	this->dwRoomID = this->pRoom->dwRoomID;
	this->dwPlayerID = g_pTheDB->GetPlayerID();

	UnpackData(this->stats);

	//There should be no commands at the beginning of the room, unless
	//the command sequence is being replayed.
	if (!this->Commands.IsFrozen())
		this->Commands.Clear();

/*
	//No room play snapshots should exist when restarting room.
	delete this->pSnapshotGame;
	this->pSnapshotGame = NULL;
	this->dwComputationTime = 0;
*/
}

//*****************************************************************************
bool CCurrentGame::SetRoomAtCoords(
//Loads a room on this level and sets the current room to it.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in)   Coords to specify room to load.
//
//Returns:
//True if successful, false if not.  If a failure occurs, the current room will
//stay loaded.
{
	//Load new room.
	CDbRoom *pNewRoom = this->pLevel->GetRoomAtCoords(dwRoomX, dwRoomY);
	if (!pNewRoom)
		return false;

	ASSERT(!this->Commands.IsFrozen() || IsValidatingPlayback());
	//NOTE: If commands can actually be frozen here, then logic such as found
	//in ProcessPlayer_HandleLeaveRoom is needed.

	ExitCurrentRoom();

	//Free the old room and return successful.
	delete this->pRoom;
	this->pRoom = pNewRoom;
	return true;
}

//*****************************************************************************
void CCurrentGame::SetRoomStartToPlayer()
//Opposite of SetPlayerToRoomStart().
{
	this->wStartRoomAppearance = this->pPlayer->wIdentity;
	CDbSavedGame::bStartRoomSwordOff = this->pPlayer->bSwordOff;
	this->wStartRoomO = this->pPlayer->wO;
	this->wStartRoomX = this->pPlayer->wX;
	this->wStartRoomY = this->pPlayer->wY;
	PackData(this->stats);
	this->statsAtRoomStart = this->stats;
	this->roomsExploredAtRoomStart = GetExploredRooms();
	this->roomsMappedAtRoomStart = GetMappedRooms();
}

//*****************************************************************************
void CCurrentGame::DestroyInventory(
//Destroy the player's current inventory item in the specified slot.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT type,
	const bool bShowStatChanges)
{
	CSwordsman& p = *(this->pPlayer);
	int oldATKstat, oldDEFstat;
	getEquipmentStats(type, oldATKstat, oldDEFstat);

	removeGlobalScriptForEquipment(type);

	switch (type)
	{
		case ScriptFlag::Weapon:
			p.st.sword = NoSword;
			break;
		case ScriptFlag::Armor:
			p.st.shield = NoShield;
			break;
		case ScriptFlag::Accessory:
			p.st.accessory = NoAccessory;
			break;
		default: break;
	}

	int newATKstat, newDEFstat;
	getEquipmentStats(type, newATKstat, newDEFstat);

	if (bShowStatChanges) {
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_ATK,
				newATKstat - oldATKstat), true);
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_DEF,
				newDEFstat - oldDEFstat), true);
	}
}

//*****************************************************************************
void CCurrentGame::dropOldInventory(
//When an old inventory item is being relinquished,
//place it on the player's current tile in the room if possible.
//
//Params:
	const UINT oldEquipment,  //old equipment ID being dropped
	const UINT newEquipment,  //whether the new item being grabbed is a custom item
	const UINT tItem,         //what tile type represents this inventory type
	const UINT emptyItemType) //enumeration value representing an empty inventory slot
{
	CSwordsman& p = *(this->pPlayer);
	if (!p.IsInRoom())
		return; //nowhere to place it -- the old equipment is lost forever

	//Place old equipment at the player's location.
	const UINT tTile = this->pRoom->GetTSquare(p.wX, p.wY);
	const UINT tParam = this->pRoom->GetTParam(p.wX, p.wY);
	if (tTile == tItem && newEquipment == tParam)
	{
		//New item is being picked up from the room tile.
		//Place the old equipment where the new equipment was.
		if (oldEquipment == emptyItemType)
			this->pRoom->Plot(p.wX, p.wY, T_EMPTY); //nothing replacing the acquisition
		else
			this->pRoom->SetTParam(p.wX, p.wY, oldEquipment); //this type left in place
	} else {
		//Custom equipment is being bestowed via script command,
		//and not being picked up from the current room tile.
		ASSERT(bIsCustomEquipment(newEquipment));

		if (oldEquipment != emptyItemType) //something needs to be dropped
		{
			//Receiving a custom item could cause a dropped item to overwrite other
			//equipment already at this location.
			//1. If some other inventory item of this type is at the player's
			//   location, it gets lost forever.
			//2. If some other game element occupies this tile,
			//   then the item being dropped gets lost forever.
			if (tTile == tItem || tTile == T_EMPTY)
			{
				if (tTile == T_EMPTY) //may place old equipment on an empty tile
					this->pRoom->Plot(p.wX, p.wY, tItem);
				this->pRoom->SetTParam(p.wX, p.wY, oldEquipment); //any other item on this tile is clobbered
			}
			//else: don't allow overwriting an incompatible object with my old equipment
		}
	}

	this->pRoom->Plot(CCoordSet(p.wX, p.wY));
}

//*****************************************************************************
void CCurrentGame::DisableInventory(
//Disable/enable equipment of specified type.
//
//Params:
	CCueEvents& /*CueEvents*/,
	const UINT type,       //equipment type
	const bool bDisable)   //if true [default], then disable, else re-enable
{
	CSwordsman& p = *(this->pPlayer);
	switch (type)
	{
		case ScriptFlag::Weapon: p.bSwordOff = bDisable; break;
		case ScriptFlag::Armor: p.bShieldOff = bDisable; break;
		case ScriptFlag::Accessory: p.bAccessoryOff = bDisable; break;
		case ScriptFlag::Command: p.bCommandOff = bDisable; break;
		default: break;
	}
}

//*****************************************************************************
void CCurrentGame::EnableInventory(CCueEvents& CueEvents, const UINT type)
//Re-enable equipment of specified type.
{
	DisableInventory(CueEvents, type, false);
}

//*****************************************************************************
bool CCurrentGame::QueryInventoryStatus(
//Determine whether the player is currently wielding equipment of the specified type.
//This check involves both indefinite enable/disable logic,
//as well as whether the equipment is current put away (e.g. on oremites),
//temporarily disabled (e.g. disarm token).
//
//Returns: true if equipment is enabled, otherwise false
//         NOTE that this query can return true even if the player's inventory slot is empty.
//
//Params:
	const UINT type)   //equipment type
const
{
	switch (type)
	{
		case ScriptFlag::Weapon: return !IsPlayerSwordDisabled();
		case ScriptFlag::Armor: return !IsPlayerShieldDisabled();
		case ScriptFlag::Accessory: return !IsPlayerAccessoryDisabled();
		case ScriptFlag::Command: return MayUseItem(ScriptFlag::Command);
		default: ASSERT(!"Unknown inventory type"); return false;
	}
}

//*****************************************************************************
void CCurrentGame::SellInventory(
//Sell the player's current inventory item in the specified slot.
//The player receives its monetary value and the item is destroyed.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT type,
	const bool bShowStatChanges)
{
	if (type != ScriptFlag::Command) //can't sell commands
	{
		CSwordsman& p = *(this->pPlayer);
		CCharacter* pCharacter = getCustomEquipment(type);
		if (pCharacter)
		{
			const int gold = pCharacter->getGOLD();
			incintValueWithBounds(p.st.GOLD, gold);
			if (bShowStatChanges)
				CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_GOLD, gold), true);
		}
		//predefined weapon types have no monetary value
	}

	DestroyInventory(CueEvents, type, bShowStatChanges);
}

//*****************************************************************************
void CCurrentGame::TradeAccessory(
//Trade the player's current accessory, if any, for the specified one.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT newEquipment, //ID of new equipment received
	const bool bShowStatChanges)
{
	CSwordsman& p = *(this->pPlayer);
	PlayerStats& st = p.st;
	const UINT oldEquipment = st.accessory;
	const bool bNewIsCustom = bIsCustomEquipment(newEquipment);

	if (newEquipment == oldEquipment && !bNewIsCustom)
		return; //nothing special happens when the same equipment is traded
	if (newEquipment == AccessorySlot && oldEquipment == NoAccessory)
		return; //nothing to drop in the slot

	//Custom equipment types must be identified with a valid custom character to be used.
	if (bNewIsCustom)
	{
		HoldCharacter *pChar = this->pHold->GetCharacter(newEquipment);
		if (!pChar)
			return;
	}

	int oldATKstat, oldDEFstat;
	getEquipmentStats(ScriptFlag::Accessory, oldATKstat, oldDEFstat);

	//Remove old custom item from global scripts and add the new one.
	if (changingInventory(CueEvents, ScriptFlag::Accessory, newEquipment))
	{
		CueEvents.Add(CID_ReceivedEquipment);

		dropOldInventory(oldEquipment, newEquipment, T_ACCESSORY, NoAccessory);
		st.accessory = (newEquipment == AccessorySlot ? (UINT)NoAccessory : newEquipment);

		int newATKstat, newDEFstat;
		getEquipmentStats(ScriptFlag::Accessory, newATKstat, newDEFstat);

		if (bShowStatChanges) {
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_ATK,
					newATKstat - oldATKstat), true);
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_DEF,
					newDEFstat - oldDEFstat), true);
		}
	}
}

//*****************************************************************************
void CCurrentGame::TradeArmor(
//Trade the player's current armor, if any, for the specified one.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT newEquipment, //ID of new equipment received
	const bool bShowStatChanges)
{
	CSwordsman& p = *(this->pPlayer);
	PlayerStats& st = p.st;
	const UINT oldEquipment = st.shield;
	const bool bNewIsCustom = bIsCustomEquipment(newEquipment);

	if (newEquipment == oldEquipment && !bNewIsCustom)
		return; //nothing special happens when the same equipment is traded
	if (newEquipment == ArmorSlot && oldEquipment == NoShield)
		return; //nothing to drop in the slot

	//Custom equipment types must be identified with a valid custom character to be used.
	if (bNewIsCustom)
	{
		HoldCharacter *pChar = this->pHold->GetCharacter(newEquipment);
		if (!pChar)
			return;
	}

	int oldATKstat, oldDEFstat;
	getEquipmentStats(ScriptFlag::Armor, oldATKstat, oldDEFstat);

	//Remove old custom item from global scripts and add the new one.
	if (changingInventory(CueEvents, ScriptFlag::Armor, newEquipment))
	{
		CueEvents.Add(CID_ReceivedEquipment);

		dropOldInventory(oldEquipment, newEquipment, T_SHIELD, NoShield);
		st.shield = (newEquipment == ArmorSlot ? (UINT)NoShield : newEquipment);

		int newATKstat, newDEFstat;
		getEquipmentStats(ScriptFlag::Armor, newATKstat, newDEFstat);

		if (bShowStatChanges) {
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_ATK,
					newATKstat - oldATKstat), true);
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_DEF,
					newDEFstat - oldDEFstat), true);
		}
	}
}

//*****************************************************************************
void CCurrentGame::TradeWeapon(
//Trade the player's current weapon, if any, for the specified one.
//
//Params:
	CCueEvents& CueEvents,   //(in/out)
	const UINT newEquipment, //ID of new equipment received
	const bool bShowStatChanges)
{
	CSwordsman& p = *(this->pPlayer);
	PlayerStats& st = p.st;
	const UINT oldEquipment = st.sword;
	const bool bNewIsCustom = bIsCustomEquipment(newEquipment);

	if (newEquipment == oldEquipment && !bNewIsCustom)
		return; //nothing special happens when the same equipment is traded
	if (newEquipment == WeaponSlot && oldEquipment == NoSword)
		return; //nothing to drop in the slot

	//Custom equipment types must be identified with a valid custom character to be used.
	if (bNewIsCustom)
	{
		HoldCharacter *pChar = this->pHold->GetCharacter(newEquipment);
		if (!pChar)
			return;
	}

	int oldATKstat, oldDEFstat;
	getEquipmentStats(ScriptFlag::Weapon, oldATKstat, oldDEFstat);

	//Remove old custom item from global scripts and add the new one.
	if (changingInventory(CueEvents, ScriptFlag::Weapon, newEquipment))
	{
		CueEvents.Add(CID_ReceivedEquipment);

		dropOldInventory(oldEquipment, newEquipment, T_SWORD, NoSword);
		st.sword = (newEquipment == WeaponSlot ? (UINT)NoSword : newEquipment);

		int newATKstat, newDEFstat;
		getEquipmentStats(ScriptFlag::Weapon, newATKstat, newDEFstat);

		if (bShowStatChanges) {
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_ATK,
					newATKstat - oldATKstat), true);
			CueEvents.Add(CID_EntityAffected, new CCombatEffect(&p, CET_DEF,
					newDEFstat - oldDEFstat), true);
		}

		SetPlayerSwordSheathed();
	}
}

//*****************************************************************************
/*
void CCurrentGame::TallyKill()
//Add one to kill count for current room and level.
{
	++this->wMonsterKills;

	if (!this->Commands.IsFrozen())
		++this->dwLevelKills;
}

//-****************************************************************************
void CCurrentGame::SwitchToCloneAt(const UINT wX, const UINT wY)
//Switch player and clone's positions.
{
	ASSERT(this->pRoom->IsValidColRow(wX,wY));
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX, wY);
	ASSERT(pMonster);
	ASSERT(pMonster->wType == M_CLONE);
	ASSERT(!this->pRoom->GetMonsterAtSquare(this->pPlayer->wX, this->pPlayer->wY));
	const UINT wO = pMonster->wO;
	pMonster->Move(this->pPlayer->wX, this->pPlayer->wY);
	pMonster->wO = this->pPlayer->wO;
	CClone *pClone = DYN_CAST(CClone*, CMonster*, pMonster);
	pClone->SetSwordSheathed();
	this->pPlayer->Move(wX,wY);
	this->pPlayer->SetOrientation(wO);
	SetPlayerSwordSheathed();

	//Don't show swapping movement.
	UpdatePrevCoords();

	//Keep track of last clone moved to.
	this->pRoom->pLastClone = pMonster;

	//Brain pathmap target is now at this location.
	if (this->swordsman.IsTarget())
		this->pRoom->SetPathMapsTarget(wX,wY);

	//Currently, this command can happen without anything else changing.
	//Variant: let everything else in room happen too, as if a turn was spent
	//Q: How to handle invisible player swapping with clone?
	//A: Currently, player retains invisibility after transfer (no code required for this).
}
*/

//*****************************************************************************
void CCurrentGame::ToggleSwordDisarm()
{
	this->pPlayer->bNoSword = !this->pPlayer->bNoSword;
	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL;
			pMonster = pMonster->pNext)
		switch (pMonster->wType)
		{
			case M_DECOY: case M_CLONE: case M_MIMIC:
			{
				CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
				pDouble->bNoSword = this->pPlayer->bNoSword;
			}
			break;
			default: break;
		}
}

//*****************************************************************************
bool CCurrentGame::TunnelMove(
//Move player to exit of tunnel entered.
//Search for an exit is performed axially along direction of tunnel entrance.
//
//Returns: whether tunnel was entered and another exited
//
//Params:
	const int dx, const int dy)   //(in) direction entering tunnel
{
	//If exit is found, then place player at destination.
	UINT wX, wY;
	if (!this->pRoom->TunnelGetExit(this->pPlayer->wX, this->pPlayer->wY, dx, dy, wX, wY))
	{
		this->pPlayer->wPrevX = this->pPlayer->wX;
		this->pPlayer->wPrevY = this->pPlayer->wY;
		this->pPlayer->wPrevO = this->pPlayer->wO;
		return false;
	}

	SetPlayer(wX, wY);
	return true;
}

//***************************************************************************************
/*
bool CCurrentGame::WasRoomConqueredOnThisVisit() 
//Was the current room conquered on this visit?  This should be called before the 
//player has left the room.
//
//Returns:
//True if room was conquered on this visit, false if not.
const
{
	ASSERT(this->bIsGameActive);

	if (this->pRoom->wMonsterCount) 
		return false;     //Room is still in an unconquered state.

	//No monsters left in the room.
	return !IsCurrentRoomConquered() ||
			//If the room isn't already conquered, that means there were monsters 
			//in the room when the player got here, and the player has conquered 
			//the room by killing them all and leaving the room.
			
			this->bIsNewRoom;
			//If the room is already conquered and this is a new room, that means 
			//there were no monsters when the player entered the room, so it was 
			//immediately conquered upon entrance.
}

//-**************************************************************************************
UINT CCurrentGame::WriteCurrentRoomConquerDemo()
//Writes a demo to show this room being conquered.
//
//Returns:
//DemoID of new Demos record.
{
	//Set recording information for a conquer demo.
	DEMO_REC_INFO dri;
	dri.dwDescriptionMessageID = MID_ConquerDemoDescription;
	dri.wBeginTurnNo = 0;
	dri.dwPrevDemoID = 0L;
	dri.dwFirstDemoID = 0L;
	dri.SetFlag(CDbDemo::Victory);

	//This call does the real work.
	const UINT dwDemoID = WriteCurrentRoomDemo(dri);
	if (!dwDemoID)
		return 0;

	//Demo hi-score submission for recognized holds.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	const bool bSubmitScores = pPlayer->Settings.GetVar(useInternetStr, false);
	if (bSubmitScores && this->wPlayerTurn) //don't accept 0-move demos
	{
		//Only upload demos for published holds.
		ASSERT(g_pTheNet);
		if (g_pTheNet->IsLocalHold(this->pHold->dwHoldID))
		{
			//Prepare demo for upload.
			string text;
			CIDSet ids(dwDemoID);
			if (CDbXML::ExportXML(V_Demos, ids, text, UINT(-1))) //no multi-room demos
			{
				//Uploads are to be handled by front end to avoid delay here so that
				//uploading demos in quick succession doesn't slow things down.
				CCurrentGame::demosForUpload.push(new DEMO_UPLOAD(text, this->wPlayerTurn,
						this->Commands.GetTimeElapsed(), dwDemoID));
			}
		}
	} else {
		//Otherwise flag that the player now has unsent score information.
		if (this->pHold->bCaravelNetMedia)
		{
			pPlayer->Settings.SetVar(playerScoresOld, true);
			pPlayer->Update();
		}
	}
   delete pPlayer;
	return dwDemoID;
}
*/

//***************************************************************************************
UINT CCurrentGame::WriteScoreCheckpointSave(const WSTRING& name)
//Writes a saved game record containing the saved game's stats info for upload.
//
//Returns:
//SavedGameID of new SavedGames record.
{
	if (this->bNoSaves)
		return 0; //playing a dummy game session -- don't upload scores

	//Save game with current player stats for scoring.
	CDbPackedVars tempStats = this->stats;

	//Insert the current ATK/DEF levels for the record made for score upload.
	PlayerStats st = this->pPlayer->st; //temp copy
	this->pPlayer->st.ATK = getPlayerATK();
	this->pPlayer->st.DEF = getPlayerDEF();

	PackData(this->stats); //data must be packed at current stat values in order to upload
			//score values correctly, since room move sequence will not be replayed on the server
	SaveGame(ST_ScoreCheckpoint, name);
	this->pPlayer->st = st; //revert
	this->stats = tempStats;

	if (!this->dwSavedGameID)
		return 0; //problem -- no game was saved

	//Score submission for recognized holds.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	const bool bSubmitScores = pPlayer->Settings.GetVar(useInternetStr, false);
	if (bSubmitScores)
	{
		//Only upload scores for published holds.
		ASSERT(g_pTheNet);
		if (g_pTheNet->IsLocalHold(this->pHold->dwHoldID))
		{
			//Prepare score XML for upload.
			string text;
			CIDSet ids(this->dwSavedGameID);
			if (CDbXML::ExportXML(V_SavedGames, ids, text, ST_ScoreCheckpoint))
			{
				//Uploads are to be handled by front end to avoid delay here.
				CCurrentGame::scoresForUpload.push(new SCORE_UPLOAD(text, GetScore(),
						name, this->dwSavedGameID));
			}
		}
	} else {
		//Otherwise flag that the player now has unsent score information.
		if (this->pHold->bCaravelNetMedia)
		{
			pPlayer->Settings.SetVar(playerScoresOld, true);
			pPlayer->Update();
		}
	}
   delete pPlayer;
	return this->dwSavedGameID;
}

//Only call this on a temporary object, prior to temporary room preview in the front-end.
//Returns: whether prep operation succeeded
bool CCurrentGame::PrepTempGameForRoomDisplay(const UINT roomID)
{
	SaveExploredRoomData(*this->pRoom); //so current room can be viewed while panning the map

	this->pPlayer->wIdentity = this->pPlayer->wAppearance = M_NONE; //not in room

	SetRoomStartToPlayer(); //applies current stats for displaying temp rooms

	//Get current room state.
	delete this->pRoom;
	this->pRoom = g_pTheDB->Rooms.GetByID(roomID);
	if (!this->pRoom) {
		return false;
	}

	CCueEvents Ignored;
	RestartRoom(Ignored);
	for (CMonster* pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->ResolveLogicalIdentity(this->pHold);
		}
	}

	this->pRoom->SetSwordsSheathed();

	return true;
}

//***************************************************************************************
/*
UINT CCurrentGame::WriteCurrentRoomDemo(
//Update database with demo information for current room.
//
//Note: Logic for updating demo descriptions in this method depends on this method
//being the only updater of demo descriptions.
//
//Params:
	DEMO_REC_INFO &dri,  //(in/out)  Receives with members set with info needed to
						//       record the demo.  Returns with with values that
						//       can be used in a subsequent call for a multi-room 
						//       demo.
	const bool bHidden,     //(in)      Make the demo hidden?  Default is false.
	const bool bAppendRoomLocation)  //(in) Add room location to description (default = true)
//
//Returns:
//DemoID of new Demos record.
{
	ASSERT(dri.dwDescriptionMessageID);
	if (this->wTurnNo == dri.wBeginTurnNo) //could happen if room was exited immediately on entrance
		return 0;
	if (this->bNoSaves)
		return 0;

	//Save the current game to a new slot.
	this->dwSavedGameID = 0L;
	this->eType = ST_Demo;
	this->bIsHidden = true;
	PackData(this->stats);
	Update();
	ASSERT(this->dwSavedGameID);

	//Get a new demo and set its properties.
	CDbDemo *pDemo = new CDbDemo;
	pDemo->bIsHidden = bHidden;
	pDemo->dwSavedGameID = this->dwSavedGameID;
	pDemo->wBeginTurnNo = dri.wBeginTurnNo;
	pDemo->wEndTurnNo = this->wTurnNo - 1;
	pDemo->dwFlags = dri.dwFlags;

	//Prepend room position to non-empty (player viewable) demo descriptions.
	const WCHAR *pText = g_pTheDB->GetMessageText(dri.dwDescriptionMessageID);
	WSTRING descText;
	if (bAppendRoomLocation && !dri.dwPrevDemoID)
	{
		descText = AbbrevRoomLocation();
		descText += wszColon;
		descText += wszSpace;
	}
	descText += pText;
	pDemo->DescriptionText = descText.c_str();

	if (dri.dwPrevDemoID)
	{
		WSTRING wstrNewDescription = (const WCHAR *)pDemo->DescriptionText;
		if (dri.dwPrevDemoID == dri.dwFirstDemoID) //2nd demo.
		{
			WSTRING wTmp;
			AsciiToUnicode( " (2)", wTmp );
			wstrNewDescription += wTmp;
		}
		else //3rd or later demo.
		{
			//Parse number from description text.
			WSTRING wstrNum;
			WSTRING::iterator iNumStart = wstrNewDescription.end();
			WSTRING::iterator iNumStop = wstrNewDescription.end();
			WSTRING wTmp;
			AsciiToUnicode("()", wTmp);
			WSTRING::iterator iSeek;
			for (iSeek = wstrNewDescription.end() - 1; iSeek != wstrNewDescription.begin(); --iSeek)
			{
				if ((WCHAR)*iSeek == wTmp[1]) {iNumStop = iSeek; continue;}
				if ((WCHAR)*iSeek == wTmp[0]) {iNumStart = iSeek + 1; break;}
			}
			for (iSeek = iNumStart; iSeek != iNumStop; ++iSeek)
				wstrNum += *iSeek;
			int nNum = _Wtoi(wstrNum.c_str());

			//Parsing error or previous call didn't append "(n)".
			ASSERT(nNum != 0);
			ASSERT((iNumStart != wstrNewDescription.end()) &&
					   (iNumStop != wstrNewDescription.end()));

			//Concat new description with incremented number.
			WCHAR wczNewNum[7];
			_itoW(nNum + 1, wczNewNum, 10);
			wstrNewDescription.replace(iNumStart, iNumStop, wczNewNum);
		}
		pDemo->DescriptionText = wstrNewDescription.c_str();
		dri.dwDescriptionMessageID = pDemo->DescriptionText.UpdateText();
	}
	pDemo->dwChecksum = GetChecksum();

	//Update the demo.
	pDemo->Update();
	const UINT dwNewDemoID = pDemo->dwDemoID;
	delete pDemo;
	ASSERT(dwNewDemoID);

	//If recording began in a previous room, update the demo record for that room
	//so that its NextDemoID field will point to this new demo record.
	if (dri.dwPrevDemoID)
	{
		CDbDemo *pPrevDemo = g_pTheDB->Demos.GetByID(dri.dwPrevDemoID);
		ASSERT(pPrevDemo); //If fires, then probably a bad value in dwPrevDemoID.
		ASSERT(pPrevDemo->dwNextDemoID == 0L);
		pPrevDemo->dwNextDemoID = dwNewDemoID;
		if (pPrevDemo->dwDemoID == dri.dwFirstDemoID) //First demo.
		{
			//Add "(1)" to description to indicate multi-room demo.
			WSTRING wstrNewDescription = (const WCHAR *)pPrevDemo->DescriptionText;
			WSTRING wTmp;
			AsciiToUnicode(" (1)", wTmp);
			wstrNewDescription += wTmp;
			pPrevDemo->DescriptionText = wstrNewDescription.c_str();
		}
		pPrevDemo->Update();
		delete pPrevDemo;
	}

	//If no first demo ID is stored already then this is it.
	if (!dri.dwFirstDemoID)
		dri.dwFirstDemoID = dwNewDemoID;

	return dwNewDemoID;
}
*/
