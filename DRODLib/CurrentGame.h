// $Id$

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
 * Michael Welsh Duggan (md5i), Richard Cookney (timeracer), JP Burford (jpburford),
 * John Wm. Wicks (j_wicks), Mike Rimer (mrimer)
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

#include "Clone.h"
#include "CurrentGameRecords.h"
#include "DemoRecInfo.h"
#include "DbSavedGames.h"
#include "GameConstants.h"
#include "Monster.h"
#include "MonsterMessage.h"
#include "RoomData.h"
#include "Swordsman.h"
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

class CDbLevel;
class CDbHold;

static const UINT TIRED_TURN_COUNT = 40;   //# previous turns to check for player becoming tired

struct SpeechLog
{
	SpeechLog(const WSTRING customName, CCharacterCommand* pSpeechCommand) {
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
struct TemporalSplitData
{
	TemporalSplitData();

	void activate();
	void add_move(int command, bool moved);
	void clear();
	void done_activating();
	bool start(int turn, UINT x, UINT y);
	bool queuing() const;

	int queuingTurn;
	int x, y;
	vector<int> player_commands;
};

struct MusicData
{
	MusicData() { reset(); }

	enum SongID {
		//match values in front-end
		SONGID_DEFAULT = -3, //indicates to use the default music style for this area
		SONGID_CUSTOM = -2   //accompanied by dataID to play embedded music
	};

	void reset() {
		trackID = customDataID = SONGID_DEFAULT;
		songlistKey.clear();
	}

	UINT trackID;
	UINT customDataID;
	WSTRING songlistKey;
};

struct RoomCompletionData
{
	RoomCompletionData(UINT wOriginalMonsterCount, bool bConquerTokenNeedsActivating)
		: wOriginalMonsterCount(wOriginalMonsterCount),
		bConquerTokenNeedsActivating(bConquerTokenNeedsActivating) {}

	UINT wOriginalMonsterCount;
	bool bConquerTokenNeedsActivating;
};

struct cloneComparator {
    bool operator() (const CClone *a, const CClone *b) const;
};

//*******************************************************************************
class CCharacter;
class CDb;
class CHalph;
class CArmedMonster;
class CCharacterCommand;
class CFiredCharacterCommand;
struct DEMO_UPLOAD;
class CCurrentGame : public CDbSavedGame
{
protected:
	friend class CDb;
	CCurrentGame();
	CCurrentGame(const CCurrentGame &Src)
		: CDbSavedGame(false), pRoom(NULL), pLevel(NULL),
		  pHold(NULL), pEntrance(NULL), pSnapshotGame(NULL)
	{SetMembers(Src);}

public:
	~CCurrentGame();
	CCurrentGame &operator= (const CCurrentGame &Src) {
		SetMembers(Src);
		return *this;
	}

	WSTRING  AbbrevRoomLocation();
	CMonster* AddNewEntity(CCueEvents& CueEvents, const UINT identity,
			const UINT wX, const UINT wY, const UINT wO);
	bool     AreBeaconsIgnored() const;
	void     BeginDemoRecording(const WCHAR* pwczSetDescription,
			const bool bUseCurrentTurnNo=true);
	void     Clear(const bool bNewGame=true);
	static void DiffVarValues(const VARMAP& vars1, const VARMAP& vars2, set<VarNameType>& diff);
	UINT     EndDemoRecording();
	bool     ExecutingNoMoveCommands() const {return this->bExecuteNoMoveCommands;}
	int EvalPrimitive(ScriptVars::PrimitiveType ePrimitive, const vector<int>& params);
	WSTRING  ExpandText(const WCHAR* wText, CCharacter *pCharacter=NULL);
	WSTRING  getTextForInputCommandKey(InputCommands::DCMD id) const;
	void     FegundoToAsh(CMonster *pMonster, CCueEvents &CueEvents);
	void     FreezeCommands();
	UINT     GetAutoSaveOptions() const {return this->dwAutoSaveOptions;}
	UINT     GetChecksum() const;
	int      GetCutSceneStartTurn() const {return this->cutSceneStartTurn;}
	const CEntity* GetDyingEntity() const {return this->pDyingEntity;}
	const CEntity* GetKillingEntity() const {return this->pKillingEntity;}
	void     GetLevelStats(CDbLevel *pLevel);
	MusicData GetMusic() const { return music; }
	bool     GetNearestEntranceForHorn(UINT wHornX, UINT wHornY, UINT wSummonType, UINT& wX, UINT& wY);
	UINT     GetNextImageOverlayID();
	UINT     GetRoomExitDirection(const UINT wMoveO) const;
	WSTRING  GetScrollTextAt(const UINT wX, const UINT wY);
	CEntity* getSpeakingEntity(CFiredCharacterCommand* pFiredCommand);
	bool     GetSwordsman(UINT& wSX, UINT& wSY, const bool bIncludeNonTarget=false) const;
	UINT     GetSwordMovement() const
			{return swordsman.wSwordMovement;}  //note: set in ProcessPlayer
	TemporalSplitData GetTemporalSplit() const { return this->temporalSplit; }
	const CMonsterMessage* GetUnansweredQuestion() const
			{ return UnansweredQuestions.empty() ? NULL : &UnansweredQuestions.front(); }
	UINT     getVar(const UINT varIndex) const;
	void     GetVarValues(VARMAP& vars);
	WSTRING  getStringVar(const UINT varIndex) const;
	void     GotoLevelEntrance(CCueEvents& CueEvents, const UINT wEntrance, const bool bSkipEntranceDisplay=false);
	bool     IsCurrentLevelComplete() const;
	bool     IsCurrentRoomConquered() const;
	bool     IsCurrentRoomPendingExit() const;
	bool     IsCurrentRoomPendingConquer() const;
	bool     IsCurrentRoomExplored() const;
	bool     IsCutScenePlaying() const {return this->dwCutScene && !this->swordsman.wPlacingDoubleType;}
	bool     IsDemoRecording() const {return this->bIsDemoRecording;}
	bool     IsMusicStyleFrozen() const {return this->bMusicStyleFrozen;}
	bool     IsNewRoom() const {return this->bIsNewRoom;}
	bool     IsPlayerAnsweringQuestions() const {return this->UnansweredQuestions.size() != 0;}
	bool     IsPlayerAt(const UINT wX, const UINT wY) const;
	bool     IsPlayerEntranceValid() const;
	bool     IsPlayerDying() const;
	bool     IsPlayerWeaponAt(const UINT wX, const UINT wY, const bool bIgnoreDagger = false) const;
	bool     IsPlayerWading() const;
	bool     IsRoomAtCoordsConquered(const UINT dwRoomX, const UINT dwRoomY) const;
	bool     IsRoomAtCoordsExplored(const UINT dwRoomX, const UINT dwRoomY) const;
	static bool IsSupportedPlayerRole(const UINT wType);
	bool     ShouldSaveRoomBegin(const UINT dwRoomID) const;
	bool     LoadFromHold(const UINT dwHoldID, CCueEvents &CueEvents);
	bool     LoadFromLevelEntrance(const UINT dwEntranceID,	CCueEvents &CueEvents);
	bool     LoadFromWorldMap(const UINT worldMapID);
	bool     LoadFromRoom(const UINT dwRoomID, CCueEvents &CueEvents,
			const UINT wX, const UINT wY, const UINT wO, const bool bNoSaves=false);
	bool     LoadFromSavedGame(const UINT dwSavedGameID, CCueEvents &CueEvents,
			bool bRestoreAtRoomStart = false, const bool bNoSaves=false);
	bool     LoadNewRoomForExit(const UINT wExitO, CCueEvents &CueEvents);
	void     LoadPrep(const bool bNewGame=true);
	bool     PlayAllCommands(CCueEvents &CueEvents,
			const bool bTruncateInvalidCommands=false);
	bool     PlayCommandsToTurn(const UINT wEndTurnNo, CCueEvents &CueEvents);
	bool     PlayerEnteredTunnel(const UINT wOTileNo, const UINT wMoveO, UINT wRole = M_NONE) const;
	void     PostProcessCharacter(CCharacter* pCharacter, CCueEvents& CueEvents);
	void     ProcessCommandSetVar(const UINT itemID, UINT newVal);
	void     ProcessCommand(int nCommand, CCueEvents &CueEvents,
			UINT wX=(UINT)-1, UINT wY=(UINT)-1);
	void     ProcessArmedMonsterWeapon(CArmedMonster* pArmedMonster, CCueEvents& CueEvents);
	void     ProcessPlayerWeapon(int dx, int dy, CCueEvents& CueEvents);
	void     ProcessPlayerMoveInteraction(int dx, int dy, CCueEvents& CueEvents,
			const bool bWasOnSameScroll, const bool bPlayerMove = true, const bool bPlayerTeleported = false);
	void     ProcessScriptedPush(const WeaponStab& push, CCueEvents& CueEvents, CCharacter* pCharacter);
	void     ProcessWeaponHit(const UINT wX, const UINT wY, CCueEvents &CueEvents,
			CArmedMonster *pArmedMonster = NULL);
	void     QueryCheckpoint(CCueEvents& CueEvents, const UINT wX, const UINT wY) const;
	void     ReplaceDoubleCommands();
	void     RestartRoom(CCueEvents &CueEvents);
	void     RestartRoomFromLastCheckpoint(CCueEvents &CueEvents);
	void     RestartRoomFromLastDifferentCheckpoint(CCueEvents &CueEvents);
	void     SaveToCheckpoint();
	void     SaveToContinue();
	void     SaveToEndHold();
	void     SaveToHoldMastered();
	void     SaveToLevelBegin();
	void     SaveToRoomBegin();
	void     SaveToWorldMap();
	void     ScriptCompleted(const CCharacter* pCharacter);
	void     SetAutoSaveOptions(const UINT dwSetAutoSaveOptions);
	void     SetCloneWeaponsSheathed();
	void     SetComputationTimePerSnapshot(const UINT dwTime);
	void     SetCurrentRoomConquered();
	void     SetCurrentRoomExplored();
	bool     SetDyingEntity(const CEntity* pDyingEntity, const CEntity* pKillingEntity=NULL);
	void     SetExecuteNoMoveCommands(const bool bVal=true) {this->bExecuteNoMoveCommands = bVal;}
	void     SetMusicStyle(const MusicData& newMusic, CCueEvents& CueEvents);
	void     SetPlayer(const UINT wSetX, const UINT wSetY);
	void     TeleportPlayer(const UINT wSetX, const UINT wSetY, CCueEvents& CueEvents);
	void     SetPlayerAsTarget();
	void     SetPlayerRole(const UINT wType, CCueEvents& CueEvents);
	bool     SetPlayerWeaponSheathedState();
	bool     SetPlayerToEastExit();
	bool     SetPlayerToNorthExit();
	bool     SetPlayerToSouthExit();
	bool     SetPlayerToWestExit();
	void     SetRoomStatusFromAllSavedGames();
	void     SetRoomVisited() {this->bIsNewRoom = false;} //back door
	void     SetTurn(UINT wSetTurnNo, CCueEvents &CueEvents);
	bool     ShowLevelStart() const;
	void     StabRoomTile(const WeaponStab& stab, CCueEvents& CueEvents);
	bool     StartTemporalSplit();
	void     SynchClonesWithPlayer(CCueEvents& CueEvents);
	void     CheckTallyKill(const CMonster* pMonster);
	void     CheckTallyKill(const UINT wType);
	void     TallyKill();
	bool     TunnelGetExit(const UINT wStartX, const UINT wStartY,
		const int dx, const int dy, UINT& wX, UINT& wY, const CMonster* pMonster=NULL) const;
	void     UndoCommand(CCueEvents &CueEvents);
	void     UndoCommands(const UINT wUndoCount, CCueEvents &CueEvents);
	void     UnfreezeCommands();
	void     UpdateHoldMastery(CCueEvents& CueEvents);
	UINT     UpdateTime(const UINT dwTime=0);
	bool     WalkDownStairs();
	UINT     WriteCurrentRoomDieDemo();

	//World map.
	void     SetWorldMapIcon(UINT worldMapID, UINT xPos, UINT yPos, UINT entranceID, UINT customCharID, UINT imageID, UINT displayFlags);
	void     SetWorldMapMusic(UINT worldMapID, const WSTRING& songlist);
	void     SetWorldMapMusic(UINT worldMapID, const UINT songID, const UINT customID);
	WorldMapMusic GetWorldMapMusic(UINT worldMapID);

	CDbRoom *   pRoom;
	CDbLevel *  pLevel;
	CDbHold *   pHold;
	CEntranceData *pEntrance;

	//Player state
	CSwordsman swordsman;

	//Game state vars
	bool     bSwordsmanOutsideRoom;
	bool     bIsDemoRecording;
	bool     bIsLeavingLevel;
	bool     bIsGameActive;
	UINT     wTurnNo;
	UINT     wPlayerTurn;      //player move #
	UINT     wSpawnCycleCount; //monster move #
	bool     bHalfTurn;        //half a turn taken 
	bool     bBrainSensesSwordsman;
	UINT     wLastCheckpointX, wLastCheckpointY;
	vector<UINT> checkpointTurns; //turn #s a checkpoint was activated
	UINT    dwStartTime;   //keeps track of real play time elapsed
	bool     bHoldCompleted; //whether player has completed the hold being played
	bool     bHoldMastered; //whether player has mastered hold being played
	UINT    dwCutScene;    //if set, play cut scene moves at specified rate
	bool     bContinueCutScene;
	bool     bWaitedOnHotFloorLastTurn;
	CDbPackedVars statsAtRoomStart; //stats when room was begun
	vector<CMoveCoordEx> ambientSounds;  //ambient sounds playing now
	vector<SpeechLog> roomSpeech; //speech played up to this moment in the current room
	bool     bRoomExitLocked; //safety to prevent player from exiting room when set
	UINT     conquerTokenTurn; //turn player touched a Conquer token

	UINT     wMonsterKills; //total monsters killed in current room
	UINT     wMonsterKillCombo; //total monsters killed without interruption

	//Front-end effects.
	WSTRING  customRoomLocationText;
	int      displayFilter;
	int      threatClockDisplay;
	int      playerLight, playerLightType;
	int      scriptReturnX, scriptReturnY, scriptReturnF;
	vector<CImageOverlay> persistingImageOverlays;

	//Internet.
	static queue<DEMO_UPLOAD*> demosForUpload;
	CIDSet   GlobalScriptsRunning;      //currently running global scripts, saved on room exit

protected:
	void     PostSave(const bool bConqueredOnEntrance, const bool bExploredOnEntrance);
	bool     SavePrep(bool& bExploredOnEntrance);

private:
	void     ActivateTemporalSplit(CCueEvents& CueEvents);
	void     AddCompletedScripts();
	void     AddRoomsToPlayerTally();
	void     AddQuestionsToList(CCueEvents &CueEvents,
			list<CMonsterMessage> &QuestionList) const;
	void     AddTemporalSplitCommand(int nCommand, bool moved);
	void     AmbientSoundTracking(CCueEvents &CueEvents);
	void     BlowHorn(CCueEvents &CueEvents, const UINT wSummonType,
						const UINT wHornX, const UINT wHornY);
	bool     CanSwitchToClone() const;
	bool     ContinueQueuingTemporalSplitMoves() const;
	void     DeleteLeakyCueEvents(CCueEvents &CueEvents);
	void     DrankPotion(CCueEvents &CueEvents, const UINT wDoubleType,
							const UINT wPotionX, const UINT wPotionY);
	void     FlagChallengesCompleted(CCueEvents &CueEvents);
	bool     IsActivatingTemporalSplit() const;
	bool     IsSwordsmanTired();
	void     LoadNewRoomForExit(const UINT dwNewSX, const UINT dwNewSY,
			CDbRoom* pNewRoom, CCueEvents &CueEvents, const bool bSaveGame);
	bool     LoadEastRoom();
	bool     LoadNorthRoom();
	bool     LoadSouthRoom();
	bool     LoadWestRoom();
	bool     PlayerCanExitRoom(const UINT wDirection, UINT &dwNewSX,
			UINT &dwNewSY, CDbRoom* &pNewRoom);
	void     ProcessCheckpointActivation(CCueEvents& CueEvents);
	void     ProcessDoublePlacement(int nCommand, CCueEvents &CueEvents,
			const UINT wX, const UINT wY);
	void     PreprocessMonsters(CCueEvents &CueEvents);
	void     ProcessMonsters(int nLastCommand, CCueEvents &CueEvents);
	void     ProcessMonster(CMonster* pMonster, int nLastCommand, CCueEvents &CueEvents);
	void     ProcessReactionToPlayerMove(int nCommand, CCueEvents& CueEvents);
	void     ProcessRoomCompletion(RoomCompletionData roomCompletionData, CCueEvents& CueEvents);
	void     ProcessPlayer(const int nCommand, CCueEvents &CueEvents);
	void     ProcessPlayer_HandleLeaveLevel(CCueEvents &CueEvents,
			const LevelExit& exit=LevelExit(), const bool bSkipEntranceDisplay=false);
	bool     ProcessPlayer_HandleLeaveRoom(const UINT wMoveO,
			CCueEvents &CueEvents);
	void     ProcessUnansweredQuestions(const int nCommand,
			list<CMonsterMessage> &UnansweredQuestions, CCueEvents &CueEvents);
	void     ProcessWeaponPush(const WeaponStab& push, int dx, int dy,
			const OrbActivationType eActivationType, CCueEvents &CueEvents, CArmedMonster *pArmedMonster = NULL);
	bool     PushPlayerInDirection(int dx, int dy, CCueEvents &CueEvents);
	bool     RemoveInvalidCommand(const CCueEvents& CueEvents);
	void     RemoveClearedImageOverlays(const int clearLayers, const int clearGroup = ImageOverlayCommand::NO_GROUP);
	void     ResetCutSceneStartTurn() { cutSceneStartTurn = -1; }
	void     ResetPendingTemporalSplit(CCueEvents& CueEvents);
	void     ResetTemporalSplitQueuingIfInvalid(CCueEvents& CueEvents);
	void     ResolveSimultaneousTarstuffStabs(CCueEvents &CueEvents);
	void     RoomEntranceAsserts();
	void     SaveExitedLevelStats();
	void     SetMembers(const CCurrentGame &Src);
	void     SetMembersAfterRoomLoad(CCueEvents &CueEvents, const bool bResetCommands=true, const bool bInitialEntrance=true);
	void     SetPlayerMood(CCueEvents &CueEvents);
	void     SetPlayerToRoomStart();
	bool     SetRoomAtCoords(const UINT dwRoomX, const UINT dwRoomY);
	void     SetRoomStartToPlayer();
	bool     ShouldFreezeCommandsDuringSetTurn() const;
	void     SnapshotGameState();
	void     StabMonsterAt(const WeaponStab& stab, CCueEvents& CueEvents);
	void     StashPersistingEvents(CCueEvents& CueEvents);
	void     SubmitCurrentGameScoringDemo(const UINT dwDemoID, CDbDemo::DemoFlag flag);
	bool     SwitchToCloneAt(const UINT wX, const UINT wY);
	bool     TakeSnapshotNow() const;
	void     TallyMonsterKilledByPlayerThisTurn();
	bool     ToggleGreenDoors(CCueEvents& CueEvents);
	bool     TunnelMove(const int dx, const int dy);
	void     UpdatePrevCoords();
	void     UpdatePrevPlatformCoords();
	void     UpdatePrevTLayerCoords();
	void     UploadExploredRoom();
	bool     WasRoomConqueredOnThisVisit() const;
	void     WeaponPushback(const WeaponStab& push, const int dx, const int dy,
			const OrbActivationType eActivationType, CCueEvents& CueEvents, CArmedMonster *pArmedMonster = NULL);
	UINT    WriteCompletedChallengeDemo(const set<WSTRING>& challengesCompleted=set<WSTRING>());
	UINT    WriteCurrentRoomConquerDemo();
	UINT    WriteCurrentRoomScoringDemo(CDbDemo::DemoFlag flag, const set<WSTRING>& challengesCompleted=set<WSTRING>());
	UINT    WriteCurrentRoomDemo(DEMO_REC_INFO &dri, const bool bHidden=false,
			const bool bAppendRoomLocation=true, const UINT overwriteDemoID=0);

	//"swordsman exhausted/relieved" event logic
	unsigned char monstersKilled[TIRED_TURN_COUNT]; //rolling sum of monsters killed in recent turns
	UINT     wMonstersKilledRecently;
	bool     bLotsOfMonstersKilled;

	DEMO_REC_INFO        DemoRecInfo;
	list<CMonsterMessage>   UnansweredQuestions;
	bool              bIsNewRoom;
	bool     bExecuteNoMoveCommands;
	UINT     wAddNewEntityDepth; //track in order to limit recursive depth of AddNewEntity

	TemporalSplitData temporalSplit;
	int activatingTemporalSplit;

	const CEntity *pDyingEntity, *pKillingEntity; //records entities that caused the first gameover for turn

	UINT     dwAutoSaveOptions;
	CIDSet   CompletedScriptsPending;   //saved permanently on room exit
	bool     bNoSaves;   //don't save anything to DB when set (e.g., for dummy game sessions)
	bool     bWasRoomConqueredAtTurnStart;

	CCurrentGame *pSnapshotGame; //for optimized room rewinds
	UINT dwComputationTime; //time required to process game moves up to this point
	UINT dwComputationTimePerSnapshot; //real movement computation time between game state snapshots
	UINT numSnapshots;

	int cutSceneStartTurn; //optimization: for precise front-end cut scene undo

	MusicData music;
	bool bMusicStyleFrozen;	//don't change music mood in front-end when set

	UINT imageOverlayNextID;
};

#endif //...#ifndef CURRENTGAME_H
