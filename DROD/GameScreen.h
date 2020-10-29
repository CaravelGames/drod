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

struct UndoTracking {
	UndoTracking()
		: wMaxUndos(1)
		, wConsecutiveUndoTurnThreshold(0)
		, wForbidUndoBeforeTurn(0)
	{ }

	void advanceTurnThreshold(UINT turn);
	void setRewindLimit(UINT turn);

	bool unlimited() const;
	bool canUndoBefore(UINT turn) const;

	UINT wMaxUndos; //# of turns player can undo
	UINT wConsecutiveUndoTurnThreshold; //can undo until this turn
	UINT wForbidUndoBeforeTurn; //don't allow undoing past this turn
};

//***************************************************************************************
class CFaceWidget;
class CClockWidget;
class CFiredCharacterCommand;
class CEntranceSelectDialogWidget;
class CRoomEffectList;
class CSubtitleEffect;
struct VisualEffectInfo;
class CGameScreen : public CRoomScreen
{
public:
	CCurrentGame * GetCurrentGame() const {return this->pCurrentGame;}
	CFaceWidget*   GetFaceWidget() const {return this->pFaceWidget;}
	void     GotoEntrance(UINT entranceID);
	bool     IsGameLoaded() const {return this->pCurrentGame!=NULL;}
	bool     IsPlaytesting() const { return this->bPlayTesting; }
	bool     LoadContinueGame();
	bool     LoadNewGame(const UINT dwHoldID);
	bool     LoadSavedGame(const UINT dwSavedGameID, bool bRestoreFromStart = false, const bool bNoSaves = false);
	void     MarkCurrentEntranceExplored();
	static WSTRING PrintRank(const int nRanking, const bool bTie);
	SCREENTYPE     ProcessCommand(const int nCommand, const UINT wX, const UINT wY);
	void     SetGameAmbience(const bool bRecalc=false);
	void     SetMusicStyle();
	SCREENTYPE SelectGotoScreen();
	void     SyncMusic(const bool bAllowSyncDefault=true);
	bool     TestRoom(const UINT dwRoomID, const UINT wX, const UINT wY,
			const UINT wO);
	bool     UnloadGame();

protected:
	friend class CDrodScreenManager;

	CGameScreen(const SCREENTYPE eScreen=SCR_Game);
	virtual ~CGameScreen();

	void           AddSoundEffect(const VisualEffectInfo* pEffect);
	virtual void   ApplyINISettings();
	virtual void   ChatPolling(const UINT tagUserList);
	void           ClearCueEvents();
	void           ClearSpeech(const bool bForceClearAll=true, const bool bKeepLoopingAmbientSounds=false);
	virtual void   DisplayChatText(const WSTRING& text, const SDL_Color& color);
	void           DisplayPersistentEffects();
	void           DrawCurrentTurn();
	virtual void   OnBetweenEvents();
	virtual void   OnDeactivate();
	virtual void   OnSelectChange(const UINT dwTagNo);
	virtual void   OnWindowEvent(const SDL_WindowEvent& wevent);
	virtual void   OnWindowEvent_GetFocus();
	virtual void   OnWindowEvent_LoseFocus();
	virtual void   Paint(bool bUpdateRect=true);
	void           PaintClock(const bool bShowImmediately=false);
	void           PlaySoundEffect(const UINT eSEID, float* pos=NULL, float* vel=NULL,
			const bool bUseVoiceVolume=false, float frequencyMultiplier=1.0f);
	SCREENTYPE     ProcessCommand(const int nCommand, const bool bMacro=false);
	bool           ProcessExitLevelEvents(CCueEvents& CueEvents, SCREENTYPE& eNextScreen);
	void           ProcessFuseBurningEvents(CCueEvents& CueEvents);
	void           ProcessMovieEvents(CCueEvents& CueEvents);
	void           ProcessQuestionPrompts(CCueEvents& CueEvents, SCREENTYPE& eNextScreen);
	void           RetainEffectCleanup(const bool bVal=false);
	virtual bool   SetForActivate();
	virtual bool   UnloadOnDeactivate() const {return false;}

	//These are called by CDemoScreen.
	void           SetSignTextToCurrentRoom();

	//These are accessed by CDemoScreen.
	CCurrentGame * pCurrentGame;
	CRoomWidget *  pRoomWidget;
	CCueEvents     sCueEvents; //declared static to retain cue events fired when going between screens

private:
	void           AddBloodEffect(const CMoveCoord& coord, const UINT wAppearance);
	void           AddComboEffect(CCueEvents& CueEvents);
	void           AddDamageEffect(const UINT wMonsterType, const CMoveCoord& coord, const bool bIsCriticalNpc = false);
	void           AddRoomStatsDialog();
	void           AmbientSoundSetup();
	void           ApplyPlayerSettings();
	bool           CanShowVarUpdates();
	void           ClearEventsThatOnlyShowOnInitialRoomEntrance(CCueEvents& CueEvents);
	void           CutSpeech(const bool bForceClearAll=true);
	void           DeleteCurrentGame();
	void           DisplayRoomStats();
	void           FadeRoom(const bool bFadeIn, const Uint32 dwDuration, CCueEvents& CueEvents);
	UINT           GetEffectDuration(const UINT baseDuration) const;
	WSTRING        GetGameStats(const bool bHoldTotals=false, const bool bOnlyCurrentGameRooms=false) const;
	UINT           GetMessageAnswer(const CMonsterMessage *pMsg);
	UINT           GetParticleSpeed(const UINT baseSpeed) const;
	UINT           GetPlayerClearSEID() const;
	void           GotoHelpPage();
	void           HandleEventsForHoldExit();
	SCREENTYPE     HandleEventsForLevelExit();
	int            HandleEventsForPlayerDeath(CCueEvents &CueEvents);
	void           HideBigMap();
	SCREENTYPE     LevelExit_OnKeydown(const SDL_KeyboardEvent &KeyboardEvent);
	void           LogHoldVars();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseMotion(const UINT dwTagNo, const SDL_MouseMotionEvent &MotionEvent);
	void           OpenStatsBox();
	void           PlayAmbientSound(const UINT dwDataID, const bool bLoop,
			const UINT wX, const UINT wY);
	void           PlayDeathSound(const UINT wAppearance);
	void           PlaySpeakerSoundEffect(const UINT eSEID, float* pos=NULL, float* vel=NULL) const;
	void           PrepCustomSpeaker(CFiredCharacterCommand *pCmd);
	SCREENTYPE     ProcessCueEventsAfterRoomDraw(CCueEvents &CueEvents);
	SCREENTYPE     ProcessCueEventsBeforeRoomDraw(CCueEvents &CueEvents);
	void           ProcessSpeech();
	void				ProcessSpeechCues(CCueEvents& CueEvents);
	bool           ProcessSpeechSpeaker(CFiredCharacterCommand *pCommand);
	void           ReattachRetainedSubtitles();
	void           RestartRoom(int nCommand, CCueEvents& CueEvents);
	void           RetainImageOverlay(const bool bVal);
	void           RetainImageOverlay(CRoomEffectList *pEffectList, const bool bVal);
	void           RetainSubtitleCleanup(const bool bVal);
	void           SendAchievement(const string& achievement);
	bool           ShouldShowLevelStart();
	void           ShowBigMap();
	virtual void   ShowChatHistory(CEntranceSelectDialogWidget* pBox);
	void           ShowDemosForRoom(const UINT roomID);
	void           ShowLockIcon(const bool bShow=true);
	void           UpdatePlayerFace();
	void           ResolvePlayerFace(SPEAKER& pSpeaker, HoldCharacter **playerHoldCharacter);
	void           ShowRoom(CCurrentGame *pGame, CCueEvents& CueEvents);
	void           ShowRoomTemporarily(const UINT roomID);
	void           ShowSpeechLog();
	void           SwirlEffect();
	void           StopAmbientSounds(const bool bKeepLooping=false);
	void           SynchScroll();
	void           ToggleBigMap();
	void           UndoMove();
	void           UpdateUIAfterRoomRestart();
	void           UpdateUIAfterMoveUndo();
	void           UpdateScroll();
	void           UpdateSign();
	void           UpdateSound();
	bool           UploadDemoPolling();
	void           UploadPlayerProgressToCloud(const UINT holdID);
	void           UploadExploredRooms(const SAVETYPE eSaveType=ST_Continue);
	void           WaitToUploadDemos();

	bool        bShowLevelStartBeforeActivate;
	bool        bPersistentEventsDrawn;
	bool        bNeedToProcessDelayedQuestions;
	bool        bShowingBigMap;
	bool        bShowingCutScene;
	bool        bIsDialogDisplayed;

	bool        bAutoUndoOnDeath;

	CFaceWidget   *pFaceWidget;
	CClockWidget  *pClockWidget;
	CDialogWidget *pMenuDialog;
	CEntranceSelectDialogWidget *pSpeechBox;
	CMapWidget *pBigMapWidget;

	//Speech.
	deque<CFiredCharacterCommand*> speech; //speech dialog queued to play
	Uint32 dwNextSpeech; //time next speech can begin
	bool   bShowingSubtitlesWithVoice;	//whether subtitles are shown when voices are playing
	Uint32 dwTimeMinimized; //time game is minimized
	Uint32 dwLastCutSceneMove, dwSavedMoveDuration;

	struct ChannelInfo {
		ChannelInfo()
			: wChannel(0), bUsingPos(false), bLoop(false)
			, turnNo(0), scriptID(0), commandIndex(0)
			, pEffect(NULL)
		{
			pos[0] = pos[1] = pos[2] = 0.0;
		}
		ChannelInfo(const UINT channel, const bool bUsingPos=false, const bool bLoop=false,
				const float posX=0.0, const float posY=0.0, const float posZ=0.0,
				const UINT turnNo=0, const UINT scriptID=0, const UINT commandIndex=0)
			: wChannel(channel), bUsingPos(bUsingPos), bLoop(bLoop)
			, turnNo(turnNo), scriptID(scriptID), commandIndex(commandIndex)
			, pEffect(NULL)
		{
			this->pos[0] = posX; this->pos[1] = posY; this->pos[2] = posZ;
		}
		UINT wChannel;
		bool bUsingPos, bLoop;
		float pos[3];
		UINT turnNo, scriptID, commandIndex;
		CSubtitleEffect *pEffect;
		WSTRING text;
	};
	vector<ChannelInfo> speechChannels;   //audio channels speech sounds are playing on
	vector<ChannelInfo> ambientChannels;  //audio channels ambient sounds are playing on

	bool        bIsSavedGameStale;
	bool        bPlayTesting;  //from editor
	bool        bRoomClearedOnce; //has room cleared event fired for this room
	bool        bSkipCutScene;

	UndoTracking undo;

	float *fPos;   //position vector

	//Internet uploading.
	UINT wUploadingDemoHandle;
	UINT dwUploadingDemo;

	//Auto-help detection.
	Uint32 dwTimeInRoom, dwLastTime;
	Uint32 dwTotalPlayTime;
};

#endif //...#ifndef GAMESCREEN_H
