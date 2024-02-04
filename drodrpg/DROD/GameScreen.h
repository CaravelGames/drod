// $Id: GameScreen.h 9309 2008-10-29 02:52:31Z mrimer $

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

#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include "RoomScreen.h"

#include "../DRODLib/CurrentGame.h"
#include <BackEndLib/Types.h>

#include <SDL.h>

#include <queue>
using std::deque;

//***************************************************************************************
class CFaceWidget;
class CClockWidget;
class CFiredCharacterCommand;
class CEntranceSelectDialogWidget;
class CSubtitleEffect;
struct VisualEffectInfo;
class CGameScreen : public CRoomScreen
{
public:
	CCurrentGame* GetCurrentGame() const {return this->pCurrentGame;}
	bool     GetDisableMouseMovement() const {return this->bDisableMouseMovement;}
	CFaceWidget*   GetFaceWidget() const {return this->pFaceWidget;}
	SEID     GetMonsterAttackSoundEffect(CCombat* pCombat) const;
	UINT     GetMonsterDisplayTile(CMonster *pMonster, const UINT x, const UINT y);
	static MESSAGE_ID GetHintTextForRegion(int nX, int nY);
	bool     IsGameLoaded() const {return this->pCurrentGame!=NULL;}
	bool     LoadContinueGame();
	bool     LoadQuicksave();
	bool     LoadNewGame(const UINT dwHoldID);
	bool     LoadSavedGame(const UINT dwSavedGameID, bool bRestoreFromStart = false,
			const bool bNoSaves = false);
	void     MarkCurrentEntranceExplored();
	static WSTRING PrintRank(const int nRanking, const bool bTie);
	bool     ProcessCommandWrapper(const int nCommand);
	SCREENTYPE     ProcessCommand(const int nCommand, const UINT wX, const UINT wY);
	void     RedrawStats(CCombat *pCombat, const bool bUpdateRect);
	void		SetGameAmbience(const bool bRecalc=false);
	void     SetMusicStyle();
	void     SetQuickCombat();
	bool     ShouldShowLevelStart();
	void     ShowScorepointRating(int playerScore, int topScore);
	void     ShowStatsForMonster(CMonster *pMonster);
	void     ShowStatsForMonsterAt(const UINT wX, const UINT wY);
	bool     TestRoom(const UINT dwRoomID, const UINT wX, const UINT wY,
			const UINT wO, const PlayerStats& st);
	bool     UnloadGame();

protected:
	friend class CDrodScreenManager;

	CGameScreen(const SCREENTYPE eScreen=SCR_Game);
	virtual ~CGameScreen();

	void           AddVisualEffect(const VisualEffectInfo* pEffect);
	virtual void   ApplyINISettings();
	UINT           CalcTileImageForEquipment(const UINT type) const;
	bool           CanShowVarUpdates() const;
	virtual void   ChatPolling(const UINT tagUserList);
	void           ClearCueEvents();
	void           ClearSpeech(const bool bForceClearAll=false);
	void           ClickOnEquipment(const UINT eCommand, const bool bRightMouseButton);
	virtual void   DisplayChatText(const WSTRING& text, const SDL_Color& color);
	void           DisplayPersistentEffects();
	void           DrawCurrentTurn();
	WSTRING        GetEquipmentPropertiesText(const UINT eCommand);
	void           GotoHelpPage();
	virtual bool   IsDoubleClickable() const {return false;}
	void           LoadGame();
	virtual void   OnBetweenEvents();
	virtual void   OnDeactivate();
	virtual void   OnMouseDown(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnSelectChange(const UINT dwTagNo);
	virtual void   OnWindowEvent(const SDL_WindowEvent &wevent);
	virtual void   OnWindowEvent_GetFocus();
	virtual void   OnWindowEvent_LoseFocus();
	virtual void   Paint(bool bUpdateRect=true);
	void           PaintClock(const bool bShowImmediately=false);
	void           PlayHitObstacleSound(const UINT wAppearance, CCueEvents& CueEvents);
	void           PlaySoundEffect(const UINT eSEID, float* pos=NULL, float* vel=NULL,
			const bool bUseVoiceVolume=false);
	SCREENTYPE     ProcessCommand(const int nCommand); //, const bool bMacro=false);
	bool           ProcessExitLevelEvents(CCueEvents& CueEvents, SCREENTYPE& eNextScreen);
	void           ProcessFuseBurningEvents(CCueEvents& CueEvents);
	void           ProcessMovieEvents(CCueEvents& CueEvents);
	void           ProcessStatEffects(CCueEvents& CueEvents);
	void           ProcessQuestionPrompts(CCueEvents& CueEvents, SCREENTYPE& eNextScreen);
	void           RetainSubtitleCleanup(const bool bVal=false);
	void           SaveGame();
	void           SearchForPathToNextRoom(const UINT direction, CCoord *pDest=NULL);
	virtual bool   SetForActivate();
	void           ShowBattleDialog(const WCHAR* pwczMessage);
	void           ShowMonsterStats(CDbRoom *pRoom, CRoomWidget *pRoomWidget);
	virtual bool   UnloadOnDeactivate() const {return false;}

	//These are called by CDemoScreen.
	void           SetSignTextToCurrentRoom();

	//These are accessed by CDemoScreen.
	CCurrentGame * pCurrentGame;
	CRoomWidget *  pRoomWidget;
	CCueEvents     sCueEvents; //declared static to retain cue events fired when going between screens

private:
	void           AddChatDialog();
	bool           AddMonsterStats(CDbRoom* pRoom, CRoomWidget* pRoomWidget, CMonster* pMonster, WSTRING& text);
	void           AddDamageEffect(const UINT wMonsterType, const CMoveCoord& coord, float fDamagePercent=1.0f);
	void           AddKillEffect(const UINT wMonsterType, const CMoveCoord& coord);
	void           AmbientSoundSetup();
	void           ApplyPlayerSettings();
	void           CutSpeech(const bool bForceClearAll=false);
	void           DeleteCurrentGame();
	void           DisplayAdjacentTempRoom(const UINT direction);
	void           DisplayChatDialog();
//	void           DisplayRoomStats();
	void           FadeRoom(const bool bFadeIn, const Uint32 dwDuration, CCueEvents& CueEvents);
	UINT           GetEffectDuration(const UINT baseDuration) const;
//	WSTRING        GetGameStats(const bool bHoldTotals=false, const bool bOnlyCurrentGameRooms=false) const;
	UINT           GetMessageAnswer(const CMonsterMessage *pMsg);
	UINT           GetParticleSpeed(const UINT baseSpeed) const;
//	void           HandleEventsForHoldExit();
	SCREENTYPE     HandleEventsForLevelExit();
	bool           HandleEventsForPlayerDeath(CCueEvents &CueEvents);
	void           HideBigMap();
	bool           IsOpenMove(const int dx, const int dy) const;
	SCREENTYPE     LevelExit_OnKeydown(const SDL_KeyboardEvent &KeyboardEvent);
	void           LogHoldVars();
	UINT           MarkMapRoom(const UINT roomID);
	void           MovePlayerInDirection(const int dx, const int dy);
	void           MovePlayerToward(const int nScreenX, const int nScreenY);
	void           MovePlayerTowardDestTile();
	void           MovePlayerTowardTile(int nRoomX, int nRoomY);
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseMotion(const UINT dwTagNo, const SDL_MouseMotionEvent &MotionEvent);
	virtual void   OnMouseUp(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseWheel(const SDL_MouseWheelEvent &Wheel);
	void           PlayAmbientSound(const UINT dwDataID, const bool bLoop,
			const UINT wX, const UINT wY);
	void           PlaySpeakerSoundEffect(const UINT eSEID, float* pos=NULL, float* vel=NULL) const;
	void           PrepCustomSpeaker(CFiredCharacterCommand *pCmd);
	SCREENTYPE     ProcessCueEventsAfterRoomDraw(CCueEvents &CueEvents);
	SCREENTYPE     ProcessCueEventsBeforeRoomDraw(CCueEvents &CueEvents);
	void           ProcessSpeech();
	void				ProcessSpeechCues(CCueEvents& CueEvents);
	bool           ProcessSpeechSpeaker(CFiredCharacterCommand *pCommand);
	void           ReattachRetainedSubtitles();
	void           RestartRoom(int nCommand, CCueEvents& CueEvents);
	void           ScoreCheckpoint(const WCHAR* pScoreIDText);
	WSTRING        GetScoreCheckpointLine(const MID_CONSTANT statName, const UINT statAMount, const int scoreMultiplier, const UINT statScore);
	void           SendAchievement(const char* achievement, const UINT dwScore=0);
	void           ShowBigMap();
	virtual void   ShowChatHistory(CEntranceSelectDialogWidget* pBox);
	void           showStat(const UINT eType, const int delta, CEntity *pEntity, UINT& count);
//	void           ShowLockIcon(const bool bShow=true);
	void           UpdatePlayerFace();
	void           ResolvePlayerFace(SPEAKER& pSpeaker, HoldCharacter** playerHoldCharacter);
	UINT           ShowRoom(CDbRoom *pRoom, CCueEvents& CueEvents);
	void           ShowRoomCoords(CDbRoom *pRoom);
	void           ShowRoomTemporarily(UINT roomID);
	void           ShowScoreDialog(const WSTRING pTitle, const PlayerStats& st);
	void           ShowSpeechLog();
	void           SwirlEffect();
	void           SynchScroll();
	void           StopAmbientSounds();
	void           TakeStepTowardQuickExit();
	void           ToggleBigMap();
	void           UndoMove();
	void           UpdateEffectsFreeze();
	void           UpdateScroll();
	void           UpdateSign();
	void           UpdateSound();
	void           UpdateUIAfterRoomRestart(const bool bReloadEntireMap=false);
	void           UpdateUIAfterMoveUndo(bool bReloadEntireMap=false);
	bool           UploadDemoPolling();
	void           UploadExploredRooms(const SAVETYPE eSaveType=ST_Continue);
	void           WaitToUploadDemos();

	bool        bShowLevelStartBeforeActivate;
	bool        bPersistentEventsDrawn;
	bool        bNeedToProcessDelayedQuestions;
	bool        bShowingBigMap;
	bool        bShowingCutScene;
	bool        bShowingTempRoom;
	bool        bIsDialogDisplayed;
	bool        bDisableMouseMovement; //don't accept mouse movement commands
	bool        bNoMoveByCurrentMouseClick; //until mouse is clicked again
	UINT        wCombatTickSpeed, wThisCombatTickSpeed;
	UINT        wMoveDestX, wMoveDestY;  //mouse click movement destination
	UINT        wRoomQuickExitDirection; //leaving room in this direction

	//To facilitate path mapping via mouse click.
	CCoord      ignoreDestUntilMouseUp;
	CCoordSet   touchedTilesOnPath;

	CFaceWidget   *pFaceWidget;
	CClockWidget  *pClockWidget;
	CDialogWidget *pMenuDialog, *pScoreDialog;
	CEntranceSelectDialogWidget *pSpeechBox;
	CMapWidget *pBigMapWidget;
	CRoomWidget *pTempRoomWidget;

	//Speech.
	deque<CFiredCharacterCommand*> speech; //speech dialog queued to play
	Uint32 dwNextSpeech; //time next speech can begin
	bool   bShowingSubtitlesWithVoice;	//whether subtitles are shown when voices are playing
	Uint32 dwTimeMinimized; //time game is minimized
	Uint32 dwLastCutSceneMove, dwSavedMoveDuration;
	Uint32 dwLastCombatTick;

	struct ChannelInfo {
		ChannelInfo()
			: wChannel(0), bUsingPos(false)
			, turnNo(0), scriptID(0), commandIndex(0)
			, pEffect(NULL)
		{
			pos[0] = pos[1] = pos[2] = 0.0;
		}
		ChannelInfo(const UINT channel, const bool bUsingPos=false,
				const float posX=0.0, const float posY=0.0, const float posZ=0.0,
				const UINT turnNo=0, const UINT scriptID=0, const UINT commandIndex=0)
			: wChannel(channel), bUsingPos(bUsingPos)
			, turnNo(turnNo), scriptID(scriptID), commandIndex(commandIndex)
			, pEffect(NULL)
		{
			this->pos[0] = posX; this->pos[1] = posY; this->pos[2] = posZ;
		}
		UINT wChannel;
		bool bUsingPos;
		float pos[3];
		UINT turnNo, scriptID, commandIndex;
		CSubtitleEffect *pEffect;
		WSTRING text;
	};
	vector<ChannelInfo> speechChannels;   //audio channels speech sounds are playing on
	vector<ChannelInfo> ambientChannels;  //audio channels ambient sounds are playing on

	bool        bIsSavedGameStale;
	bool        bPlayTesting;  //from editor
//	bool        bRoomClearedOnce; //has room cleared event fired for this room
	UINT        wUndoToTurn; //undo moves back to this turn at once
//	bool			bHoldConquered; //whether player has conquered hold being played
//	CIDSet		roomsPreviouslyConquered; //rooms player has conquered previously in hold being played

	float *fPos;   //position vector

	//Internet uploading.
	UINT wUploadingScoreHandle;
	UINT dwUploadingDemo, dwUploadingSavedGame;

	//Auto-help detection.
//	Uint32 dwTimeInRoom, dwLastTime, dwTotalPlayTime;
};

#	ifdef STEAMBUILD
#		include <steam_api.h>

class CSteamLeaderboards
{
public:
	CSteamLeaderboards();
	~CSteamLeaderboards() {}

	void FindLeaderboardAndUploadScore(const char *pchLeaderboardName, int score);

private:
	map<string, int> m_scoresToUpload; //scores pending to upload on asynchronous OnFindLeaderboard callback

	bool FindLeaderboard(const char *pchLeaderboardName);
	bool UploadScore(SteamLeaderboard_t hLeaderboard);
	void ShowSteamScoreOnGameScreen(const char *pchLeaderboardName, int topScore);

	void OnFindLeaderboard(LeaderboardFindResult_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardFindResult_t> m_steamCallResultFindLeaderboard;

	void OnUploadScore(LeaderboardScoreUploaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoreUploaded_t> m_SteamCallResultUploadScore;

	void OnDownloadTopScore(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoresDownloaded_t> m_steamCallResultDownloadScore;
};
#	endif

#endif //...#ifndef GAMESCREEN_H
