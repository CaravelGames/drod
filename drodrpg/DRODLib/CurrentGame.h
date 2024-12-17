// $Id: CurrentGame.h 10216 2012-05-20 08:36:59Z skell $

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

//CurrentGame.h
//Declarations for CCurrentGame.h.
//
//GENERAL
//
//Class for accessing and manipulating data used for current game.  This class should
//be completely insulated from graphics, sound, and input devices.  CCurrentGame can be
//used without any UI, since there are no UI-related calls inside of it or its called
//code.
//
//USAGE
//
//Get an instance of CCurrentGame using either CDb::GetSavedCurrentGame() or
//CDb::GetNewCurrentGame().  Your commands will probably either come from a UI (user types
//commands with keyboard) or from a prerecorded game.  These commands are passed via
//CCurrentGame::ProcessCommand().  ProcessCommand() will return cue events, a full set of cues
//available from CueEvents.h.  Some cue events have to be responded to or the
//game will not appear to work, i.e. CID_MonsterKilledPlayer.  Others cue events, like
//CID_TarDestroyed, do not require handling, but can be used to cue sound and graphical
//effects.
//
//Multiple instances of CCurrentGame may be open simultaneously, but don't access
//CCurrentGame instances on different threads.
//
//DOES MY NEW METHOD GO IN CDbRoom OR CCurrentGame?
//
//This question comes up because CDbRoom and CCurrentGame both seem to lay claim to
//the same problem space.  CDbRoom is for accessing room data both before and after the
//swordsman has arrived in that room.  CCurrentGame contains the position of the swordsman,
//game state information, and an instance of CDbRoom that is the current room the swordsman
//is in.
//
//Here is the process to figure out which place the method goes:
//
//  Does the method access members of just one of the classes?  If so, then method goes
//  in that class.
//
//  Does the method need to write to a member of CCurrentGame?  If so, then method goes
//  in CCurrentGame.
//
//Assuming that the above two questions were answered "no", the method should go in CDbRoom.

#ifndef CURRENTGAME_H
#define CURRENTGAME_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DbSavedGames.h"
#include "DbDemos.h"
#include "GameConstants.h"
#include "MonsterMessage.h"
#include "PlayerDouble.h"
#include "PlayerStats.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/AttachableObject.h>
#include <BackEndLib/Coord.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Types.h>

#include <list>
#include <string>
#include <queue>
using std::list;
using std::string;
using std::queue;

//Special level exit destination values.
#define EXIT_ENDHOLD (0) //this exit ends the hold
#define EXIT_LOOKUP (-1) //level exit destination must be looked up in room record
#define EXIT_PRIOR_LOCATION (-2) //return to last location before executing a Level Entrance script warp

extern int intBounds(const float fVal);
extern UINT UINTBounds(const float fVal);
extern void incintValueWithBounds(int& val, const int inc);
extern void incUINTValueWithBounds(UINT& val, const int inc);

class CDbLevel;
class CDbHold;

//*******************************************************************************
class CCharacterCommand;
struct SpeechLog
{
	SpeechLog(WSTRING customName, CCharacterCommand* pSpeechCommand) {
		this->customName = customName;
		this->pSpeechCommand = pSpeechCommand;
	}
	SpeechLog(CCharacterCommand* pSpeechCommand) {
		this->pSpeechCommand = pSpeechCommand;
	}

	WSTRING customName;
	CCharacterCommand* pSpeechCommand;
};

//*******************************************************************************
/*
//Type used for grouping all the demo recording vars together.
struct DEMO_REC_INFO
{
	UINT dwDescriptionMessageID; //Description of demo.
	UINT  wBeginTurnNo;        //Turn at which recording began.
	UINT dwPrevDemoID;        //DemoID for a previous demo record with recording
									//for room visited before the current.  That record
									//may be updated to link to demo record storing recording
									//for current room.
	UINT dwFirstDemoID;       //DemoID of first demo in a series or demoID of the only
									//demo if there was just one.
	UINT dwFlags;          //explicit properties of demo

	void SetFlag(const CDbDemo::DemoFlag eFlag, const bool bSet=true)
	{
		ASSERT(eFlag < CDbDemo::MaxFlags);
		const UINT dwFlagVal = 1 << eFlag;
		if (bSet)
		{
			this->dwFlags |= dwFlagVal;   //set bit
		} else {
			this->dwFlags &= ~dwFlagVal;  //inverse mask to reset bit
		}
	}
};
*/

/*
//Auto-save option flags.
static const UINT ASO_NONE = 0L;
static const UINT ASO_CHECKPOINT = 1L;
static const UINT ASO_ROOMBEGIN = 2L;
static const UINT ASO_LEVELBEGIN = 4L;
static const UINT ASO_CONQUERDEMO = 8L;
static const UINT ASO_DIEDEMO = 16L;
static const UINT ASO_DEFAULT = ASO_NONE; //ASO_CHECKPOINT | ASO_ROOMBEGIN | ASO_LEVELBEGIN;
*/

//***************************************************************************************
//What kind of music is playing during the game.
struct MusicData
{
	void clear() {
		musicEnum = musicID = 0;
		songMood.resize(0);
		bPlayMusicID = false;
	}

	UINT musicEnum;    //music enum to play
	UINT musicID;      //music dataID to play
	bool bPlayMusicID; //play music for the set enumeration or ID

	WSTRING songMood;  //play music by mood name
};

//*******************************************************************************
//To facilitate some var lookup operations.
struct VarMapInfo {
	bool bInteger;
	UINT val;
	WSTRING wstrVal;
};
typedef std::string VarNameType;
typedef std::map<VarNameType, VarMapInfo> VARMAP;

struct TarstuffStab {
	TarstuffStab(const CMoveCoord& c, CMonster* m)
		: moveCoord(c), pTarstuffMonster(m)
	{ }

	CMoveCoord moveCoord;
	CMonster* pTarstuffMonster;
};

//*******************************************************************************
class CCombat;
class CDb;
class CFiredCharacterCommand;
class CSwordsman;
//struct DEMO_UPLOAD;
struct SCORE_UPLOAD;
class CCurrentGame : public CDbSavedGame
{
protected:
	friend class CDb;
	CCurrentGame();
	CCurrentGame(const CCurrentGame &Src)
		: CDbSavedGame(false), pRoom(NULL), pLevel(NULL),
		  pHold(NULL), pEntrance(NULL)//, pSnapshotGame(NULL)
	{SetMembers(Src);}

public:
	~CCurrentGame();
	CCurrentGame &operator= (const CCurrentGame &Src) {
		SetMembers(Src);
		return *this;
	}

	WSTRING  AbbrevRoomLocation();
	void     activateCustomEquipment(CCueEvents& CueEvents, const UINT type, const UINT newEquipment);
	void     ActivateTokenAt(const UINT wX, const UINT wY);
	CMonster* AddNewEntity(CCueEvents& CueEvents, const UINT identity,
			const UINT wX, const UINT wY, const UINT wO, const bool bMakeCharacterVisible=false);
	void     AddRoomToMap(const UINT roomID, const bool bMarkRoomVisible=false, const bool bSaveRoom=true);
	bool     Autosave(const WSTRING& name);
//	void     BeginDemoRecording(const WCHAR* pwczSetDescription,
//			const bool bUseCurrentTurnNo=true);
	bool     CanPlayerCutBriars() const;
	bool     CanPlayerCutTarAnywhere() const;
	bool     CanPlayerMoveTo(const UINT wX, const UINT wY) const;
	bool     changingInventory(CCueEvents& CueEvents, const UINT type, const UINT newEquipment);
	void     Clear(const bool bNewGame=true);
	bool     CustomNPCExists(const UINT characterID) const;
	void     DeserializeScriptArrays();
	void     DestroyInventory(CCueEvents& CueEvents, const UINT type, const bool bShowStatChanges=true);
	static void DiffVarValues(const VARMAP& vars1, const VARMAP& vars2, set<VarNameType>& diff);
	bool     DoesPlayerAttackFirst() const;
	bool     DoesPlayerAttackLast() const;
	bool     DoesPlayerBackstab() const;
	bool     DoesPlayerItemHaveNoEnemyDefense(const UINT type) const;
	bool     DoesTileDisableMetal(const UINT wX, const UINT wY) const;
	void     dropOldInventory(const UINT oldEquipment,	const UINT newEquipment,
			const UINT tItem, const UINT emptyItemType);
//	UINT     EndDemoRecording();
	bool     equipmentBlocksGaze(const UINT type) const;
	bool     ExecutingNoMoveCommands() const {return this->bExecuteNoMoveCommands;}
	int      EvalPrimitive(ScriptVars::PrimitiveType ePrimitive, const vector<int>& params);
	WSTRING  ExpandText(const WCHAR* wText, CCharacter *pCharacter=NULL);
	void     FreezeCommands();
//	UINT     GetAutoSaveOptions() const {return this->dwAutoSaveOptions;}
	CCharacter* GetCharacterWithScriptID(const UINT scriptID);
	UINT     GetChecksum() const;
	void     getEquipmentStats(const UINT type, int& ATKstat, int &DEFstat) const;
	float    GetGlobalStatModifier(ScriptVars::StatModifiers statType) const;
	float    GetTotalStatModifier(ScriptVars::StatModifiers statType) const;
	UINT     getNewScriptID();
	UINT     GetNextImageOverlayID();
	UINT     getSpawnID(UINT defaultMonsterID) const;
	WSTRING  getStringVar(const UINT varIndex) const;
	WSTRING  getTextForInputCommandKey(InputCommands::DCMD id) const;
	UINT     getVar(const UINT varIndex) const;
	void     GetVarValues(VARMAP& vars);
	void     GetArrayVarValues(VARMAP& vars);
	int      getItemAmount(const UINT item) const;
	int      getPlayerATK() const;
	int      getPlayerDEF() const;
	static int getPredefinedShieldPower(const UINT type);
	static int getPredefinedWeaponPower(const UINT type);
	int      getShieldPower(const UINT type) const;
	int      getWeaponPower(const UINT type) const;
	CIDSet   GetPreviouslyExploredRooms() const { return PreviouslyExploredRooms; }
	WSTRING  GetScrollTextAt(const UINT wX, const UINT wY);
	UINT     GetRoomExitDirection(const UINT wMoveO) const;
	UINT     GetScore() const;
	CEntity* getSpeakingEntity(CFiredCharacterCommand* pFiredCommand);
	bool     GetSwordsman(UINT& wSX, UINT& wSY, const bool bIncludeNonTarget=false) const;
	UINT     GetSwordMovement() const;
	const CMonsterMessage* GetUnansweredQuestion() const
			{ return UnansweredQuestions.empty() ? NULL : &UnansweredQuestions.front(); }
	void     GotoLevelEntrance(CCueEvents& CueEvents, const UINT wEntrance, const bool bSkipEntranceDisplay=false);
	bool HasUnansweredQuestion(const CMonster *pMonster) const;
	static void InitRPGStats(PlayerStats& s);
	bool     IsCurrentRoomExplored(const bool bConsiderCurrentRoom=true) const;
//	bool     IsDemoRecording() const {return this->bIsDemoRecording;}
	bool     IsEquipmentValid(const UINT id, const UINT type);
	bool     IsFighting(const CMonster* pMonster) const;
	bool     IsLuckyGRItem(const UINT type) const;
	bool     IsLuckyXPItem(const UINT type) const;
	bool     IsPlayerSwordRemoved() const;
	bool     IsNewRoom() const {return this->bIsNewRoom;}
	bool     IsPlayerAt(const UINT wX, const UINT wY) const;
	bool     IsPlayerDying() const;
	bool     IsPlayerSwordAt(const UINT wX, const UINT wY) const;
	bool     IsRoomAtCoordsExplored(const UINT dwRoomX, const UINT dwRoomY) const;
	bool     IsPlayerAccessoryDisabled() const;
	bool     IsPlayerItemDisabled(const UINT type) const;
	bool     IsPlayerShieldDisabled() const;
	bool     IsPlayerSwordDisabled() const;
	bool     IsPlayerDamagedByHotTile() const;
	bool     IsPlayerDamagedByFiretrap() const;
	bool     IsPlayerMistImmune() const;
	bool     IsPlayerSwordExplosiveSafe() const;
	bool     IsPlayerSwordWallAndMirrorSafe() const;
	bool     IsSwordMetal(const UINT type) const;
	bool     IsShieldMetal(const UINT type) const;
	bool     IsSwordStrongAgainst(const CMonster* pMonster) const;
	bool     IsEquipmentStrongAgainst(const CMonster* pMonster, const UINT type) const;
	bool     IsValidatingPlayback() const {return this->bValidatingPlayback;}
	bool     LoadFromHold(const UINT dwHoldID, CCueEvents &CueEvents);
	bool     LoadFromLevelEntrance(const UINT dwHoldID, const UINT dwEntranceID,
			CCueEvents &CueEvents);
	bool     LoadFromRoom(const UINT dwRoomID, CCueEvents &CueEvents,
			const UINT wX, const UINT wY, const UINT wO, const UINT wIdentity,
			const bool bSwordOff, const bool bNoSaves=false);
	bool     LoadFromSavedGame(const UINT dwSavedGameID, CCueEvents &CueEvents,
			bool bRestoreAtRoomStart = false, const bool bNoSaves=false);
	bool     LoadNewRoomForExit(const UINT wExitO, CCueEvents &CueEvents);
	void     LoadPrep(const bool bNewGame=true);
	bool     MayUseAccessory() const;
	bool     MayUseItem(const UINT type) const;
	bool     PlayCommands(const UINT wCommandCount, CCueEvents &CueEvents,
			const bool bTruncateInvalidCommands=false);
	void     ProcessAfterVictory(CCueEvents& CueEvents);
	void     ProcessCommand(int nCommand, CCueEvents &CueEvents,
			const UINT wX=(UINT)-1, const UINT wY=(UINT)-1);
	void     ProcessCommand_EndOfTurnEventHandling(CCueEvents& CueEvents);
	void     ProcessCommandSetVar(CCueEvents& CueEvents,
			const UINT itemID, UINT newVal);
	void     ProcessMonsterDefeat(CCueEvents& CueEvents,
			CMonster* pDefeatedMonster, const UINT wSX, const UINT wSY,
			const UINT wSwordMovement);
	void     ProcessNPCDefeat(CCharacter* pDefeatedNPC, CCueEvents& CueEvents);
	void     ProcessPostCombatExpansions(CCueEvents& CueEvents);
	void     ProcessMoveFreeScripts(CCueEvents& CueEvents, CMonster* pMonsterList);
	void     ProcessScripts(int nCommand, CCueEvents& CueEvents, CMonster* pMonsterList);
	void     ProcessSwordHit(const UINT wX, const UINT wY, CCueEvents &CueEvents,
			CMonster *pDouble = NULL);
	void     QuickSave();
	void     RemoveClearedImageOverlays(const int clearLayers);
	void     RestartRoom(CCueEvents &CueEvents);
	void     SaveGame(const SAVETYPE eSaveType, const WSTRING& name);
	void     SaveToContinue();
	void     SaveToEndHold();
/*
	void     SaveToLevelBegin();
	void     SaveToRoomBegin();
	void     SetAutoSaveOptions(const UINT dwSetAutoSaveOptions)
			{this->dwAutoSaveOptions = dwSetAutoSaveOptions;}
	void     SetComputationTimePerSnapshot(const UINT dwTime);
*/
	void     SaveExploredRoomData(CDbRoom& room);

	void     DisableInventory(CCueEvents& CueEvents, const UINT type, const bool bDisable=true);
	void     EnableInventory(CCueEvents& CueEvents, const UINT type);
	bool     QueryInventoryStatus(const UINT type) const;
	void     SellInventory(CCueEvents& CueEvents, const UINT type, const bool bShowStatChanges=true);
	void     SetExecuteNoMoveCommands(const bool bVal=true) {this->bExecuteNoMoveCommands = bVal;}
	bool     SetPlayer(const UINT wSetX, const UINT wSetY);
	void     TeleportPlayer(const UINT wSetX, const UINT wSetY, CCueEvents& CueEvents, bool bProcess = true);
	void     SetPlayerRole(const UINT wType);
	bool     SetPlayerSwordSheathed();
	void     SetTurn(UINT wSetTurnNo, CCueEvents &CueEvents);
	bool     ShowLevelStart() const;
	void     StashPersistingEvents(CCueEvents& CueEvents);
//	void     TallyKill();
	void     ToggleSwordDisarm();
	void     TradeAccessory(CCueEvents& CueEvents, const UINT newEquipment, const bool bShowStatChanges=true);
	void     TradeArmor(CCueEvents& CueEvents, const UINT newEquipment, const bool bShowStatChanges=true);
	void     TradeWeapon(CCueEvents& CueEvents, const UINT newEquipment, const bool bShowStatChanges=true);
	void     UndoCommand(CCueEvents &CueEvents);
	void     UndoCommands(const UINT wUndoCount, CCueEvents &CueEvents);
	void     UnfreezeCommands();
	void     UpdatePrevCoords();
	void     UpdateTime(const UINT dwTime=0);
	bool     UseAccessory(CCueEvents &CueEvents);
	bool     WalkDownStairs();
//	UINT     WriteCurrentRoomDieDemo();
	UINT     WriteScoreCheckpointSave(const WSTRING& name);

	bool     PrepTempGameForRoomDisplay(const UINT roomID);

	CDbRoom *   pRoom;
	CDbLevel *  pLevel;
	CDbHold *   pHold;
	CEntranceData *pEntrance;

	//Player state
	CSwordsman *pPlayer;

	//Game state vars
//	bool     bIsDemoRecording;
	bool     bIsGameActive;
	UINT     wTurnNo;
	UINT     wPlayerTurn;      //player move #
	UINT     wSpawnCycleCount; //monster move #
	bool     bHalfTurn;        //half a turn taken 
//	UINT     wMonsterKills; //total monsters killed in current room
//	bool     bBrainSensesSwordsman;
//	UINT     wLastCheckpointX, wLastCheckpointY;
//	vector<UINT> checkpointTurns; //turn #s a checkpoint was activated
	UINT     dwStartTime;   //keeps track of real play time elapsed
//	bool     bHoldMastered; //whether player has mastered hold being played
	UINT     dwCutScene;    //if set, play cut scene moves at specified rate
	bool     bContinueCutScene;
//	bool     bWaitedOnHotFloorLastTurn;
	CDbPackedVars statsAtRoomStart; //stats when room was begun
	map<UINT, map<int, int>> scriptArraysAtRoomStart;
	CIDSet   roomsExploredAtRoomStart, roomsMappedAtRoomStart;
	vector<CMoveCoordEx> ambientSounds;  //ambient sounds playing now
	vector<SpeechLog> roomSpeech; //speech played up to this moment in the current room
//	bool     bRoomExitLocked; //safety to prevent player from exiting room when set
//	PlayerStats playerStatsAtRoomStart;

	//Front-end effects.
	MusicData music;
	WSTRING  customRoomLocationText;
	vector<CImageOverlay> persistingImageOverlays;

	UINT imageOverlayNextID;

	//Internet.
//	static queue<DEMO_UPLOAD*> demosForUpload;
	static queue<SCORE_UPLOAD*> scoresForUpload;

	//Combat.
	void     MonsterInitiatesCombat(CCueEvents& CueEvents, CMonster *pMonster);
	bool     InCombat() const;
	bool     bQuickCombat; //combat resolves immediately when set
	CCombat  *pCombat;
	CMonster *pBlockedSwordHit; //when not NULL, indicates this turn's movement is invalid due to a forbidden attack on this monster

private:
	void AdvanceCombat(CCueEvents& CueEvents);
	void InitiateCombat(CCueEvents& CueEvents, CMonster *pMonster,
			const bool bPlayerHitsFirst,
			const UINT wFromX, const UINT wFromY, const UINT wX, const UINT wY,
			const bool bDefeatToStabTarTile=false);

	void		AddCompletedScripts();
	void     AddRoomsToPlayerTally();
	void     AddQuestionsToList(CCueEvents &CueEvents,
			list<CMonsterMessage> &QuestionList) const;
	void     AmbientSoundTracking(CCueEvents &CueEvents);
	void     DeleteLeakyCueEvents(CCueEvents &CueEvents);
//	void     DrankPotion(CCueEvents &CueEvents, const UINT wDoubleType);
	void     ExitCurrentRoom();
//	void     FegundoToAsh(CMonster *pMonster, CCueEvents &CueEvents);
//	UINT     FindLastCheckpointSave(const UINT wSearchUntilTurn);
	bool     KnockOnDoor(CCueEvents& CueEvents, const UINT wX, const UINT wY);
	bool     LockDoor(CCueEvents& CueEvents, const UINT wX, const UINT wY);
	void     LoadNewRoomForExit(const UINT dwNewSX, const UINT dwNewSY,
			CDbRoom* pNewRoom, CCueEvents &CueEvents);//, const bool bSaveGame);
	bool     LoadEastRoom();
	bool     LoadNorthRoom();
	bool     LoadSouthRoom();
	bool     LoadWestRoom();
	bool     LoadAdjoiningRoom(const UINT wDirection, UINT &dwNewSX,
			UINT &dwNewSY, CDbRoom* &pNewRoom);
//	void     PostSave(const bool bConqueredOnEntrance);
	void     ProcessMonsters(int nLastCommand, CCueEvents &CueEvents);
	void     ProcessTokenActivations(CCueEvents &CueEvents);
	void     ProcessPlayer(const int nCommand, CCueEvents &CueEvents);
	void     ProcessPlayerMoveInteraction(int dx, int dy, CCueEvents& CueEvents,
		const bool bWasOnSameScroll, const bool bPlayerMove = true, const bool bPlayerTeleported = false);
	void     ProcessPlayer_HandleLeaveLevel(CCueEvents &CueEvents,
			const UINT wEntrance=EXIT_LOOKUP, const bool bSkipEntranceDisplay=false);
	bool     ProcessPlayer_HandleLeaveRoom(const UINT wMoveO,
			CCueEvents &CueEvents);
	void     ProcessSimultaneousSwordHits(CCueEvents &CueEvents);
	void     ProcessUnansweredQuestions(const int nCommand,
			list<CMonsterMessage> &UnansweredQuestions, CCueEvents &CueEvents);

	void     PackData(CDbPackedVars& stats);
	void     UnpackData(CDbPackedVars& stats);
	void     RetrieveExploredRoomData(CDbRoom& room);

//	bool     SavePrep();
	void     SetMembers(const CCurrentGame &Src);
	void     SetMembersAfterRoomLoad(CCueEvents &CueEvents, const bool bResetCommands=true);
	void     SetPlayerMood(CCueEvents &CueEvents);
	void     SetPlayerToRoomStart();
	bool     SetRoomAtCoords(const UINT dwRoomX, const UINT dwRoomY);
	void     SetRoomStartToPlayer();
//	void     SwitchToCloneAt(const UINT wX, const UINT wY);
	bool     TunnelMove(const int dx, const int dy);
	void     UpdatePrevPlatformCoords();
//	UINT     WriteCurrentRoomDemo(DEMO_REC_INFO &dri, const bool bHidden=false,
//			const bool bAppendRoomLocation=true);

	vector<CCoord> pendingTokenActivations; //tokens being activated by monsters on a turn
	vector<CMoveCoord> simulSwordHits;   //vulnerable tar hit simultaneously
	vector<TarstuffStab> possibleTarStabs; //vulnerable tar hit this turn, pending monster duel
	CIDSet   changingInventoryTypes;     //to avoid recursion in trading equipment
	CMoveCoordEx *pPendingPriorLocation;

//	DEMO_REC_INFO        DemoRecInfo;
	list<CMonsterMessage>   UnansweredQuestions;
	bool              bIsNewRoom;
	bool     bExecuteNoMoveCommands;
	UINT     wAddNewEntityDepth; //track in order to limit recursive depth of AddNewEntity

//	UINT    dwAutoSaveOptions;
	CIDSet   CompletedScriptsPending;   //saved permanently on room exit

	bool     bRoomDisplayOnly; //indicates player is not in room and no player interaction should be processed
	bool     bNoSaves;   //don't save anything to DB when set (e.g., for dummy game sessions)
	bool     bValidatingPlayback; //for streamlining parts of the playback process

/*
	CCurrentGame *pSnapshotGame; //for optimized room rewinds
	UINT dwComputationTime; //time required to process game moves up to this point
	UINT dwComputationTimePerSnapshot; //real movement computation time between game state snapshots
*/

	void     AddRoomsPreviouslyExploredByPlayerToMap(UINT playerID = 0, const bool bMakeRoomsVisible = true);
	CIDSet   PreviouslyExploredRooms; //cache values
};

#endif //...#ifndef CURRENTGAME_H
