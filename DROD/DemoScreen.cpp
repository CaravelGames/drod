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
 *
 * ***** END LICENSE BLOCK ***** */

#include "HtmlDialogWidget.h"
#include "DemoScreen.h"
#include "DrodScreenManager.h"
#include "MapWidget.h"
#include "RoomWidget.h"
#include "../DRODLib/CueEvents.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbCommands.h"
#include "../DRODLib/SettingsKeys.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

const UINT FIRST_COMMAND_DELAY = 500;
const UINT LAST_COMMAND_DELAY = 500;
const UINT UNIFORM_STEP_DELAY = 1000/15; //15 tps
const UINT NAVIGATION_SPEED_INCREASE = 300; // 300 ms, see usage for explanation

float CDemoScreen::fMoveRateMultiplier = 1.0;

const UINT TAG_HELPDIALOG = 100;

//
//CDemoScreen public methods.
//

//*****************************************************************************
bool CDemoScreen::LoadDemoGame(
//Loads current game and playback info from a demo.
//
//Params:
	const UINT dwDemoID)   //(in)   Demo to load.
//
//Returns:
//True if successful, false if not.
{
	//Load the demo.
	delete this->pDemo;
	this->pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
	if (!this->pDemo) return false;

	//Load demo's saved game for CGameScreen.
	if (!CGameScreen::LoadSavedGame(this->pDemo->dwSavedGameID, true, true))
		return false;
	CGameScreen::pCurrentGame->SetAutoSaveOptions(ASO_NONE);

	//Rewind current game to beginning of demo.
	CGameScreen::pCurrentGame->SetTurn(this->pDemo->wBeginTurnNo, this->sCueEvents);

	//Update game screen widgets for new room and current game.
	if (!CGameScreen::pMapWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame) ||
			!CGameScreen::pRoomWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame)) 
		return false;

	//Don't modify any commands during demo playback.
	CGameScreen::pCurrentGame->Commands.Freeze();

	//Since there is no way to flag hold completion or mastery in a demo,
	//assume the player can go through hold completion and master walls in demos.
	//This is a safe assumption to make: if it's wrong, the player wouldn't
	//be making any moves to go through master walls anyway (observe that
	//bumps against intact master walls are not allowed in CCurrentGame::ProcessPlayer.)
	CGameScreen::pCurrentGame->bHoldCompleted = true;
	CGameScreen::pCurrentGame->bHoldMastered = true;

	CGameScreen::SetGameAmbience();
	CGameScreen::pRoomWidget->ClearEffects();
	CGameScreen::pRoomWidget->ShowPlayer();
	SetSignTextToCurrentRoom();
	CGameScreen::SetMusicStyle();
	CGameScreen::ClearSpeech();

	this->currentCommandIter = CGameScreen::pCurrentGame->Commands.Get(this->pDemo->wBeginTurnNo);
	
	return true;
}

//
//CDemoScreen protected methods.
//

//*****************************************************************************
CDemoScreen::CDemoScreen()
	: CGameScreen(SCR_Demo)
	, dwNextCommandTime(0L)
	, pDemo(NULL), bCanInteract(false)
	, bPaused(false), bPauseNextMove(false)
	, dwSavedMoveDuration(0)
	, bUniformTurnSpeed(false)
//Constructor.
{
	static const INT HELP_DIALOG_W = 800;
	static const INT HELP_DIALOG_H = 600;

	{
		CHtmlDialogWidget *pHtmlDialogWidget = new CHtmlDialogWidget(TAG_HELPDIALOG, HELP_DIALOG_W, HELP_DIALOG_H);
		this->AddWidget(pHtmlDialogWidget);

		pHtmlDialogWidget->Center();
		pHtmlDialogWidget->Hide();
	}
}

//*****************************************************************************
CDemoScreen::~CDemoScreen()
//Destructor.
{
	delete this->pDemo;
}

//******************************************************************************
bool CDemoScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	//Check that caller loaded a current game and demo.
	if (!CGameScreen::pCurrentGame || !this->pDemo)
	{
		ASSERT(!"Current game and demo not loaded.");
		return false;
	}

	//Set user display modes.
	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	if (this->pRoomWidget->IsShowingMoveCount() !=
			settings.GetVar(Settings::MoveCounter, false))
		this->pRoomWidget->ToggleMoveCount();
	if (this->pRoomWidget->IsShowingPuzzleMode() !=
			settings.GetVar(Settings::PuzzleMode, false))
		this->pRoomWidget->TogglePuzzleMode();

	SetShowVoicedSubtitle(settings.GetVar(Settings::ShowSubtitlesWithVoices, true));

	PaintClock(true);

	//Set frame rate as high as needed for smooth animations.
	SetBetweenEventsInterval(12);

	//Reset vars.
	this->bPaused = this->bPauseNextMove = false;
	this->pRoomWidget->ShowPlayer();

	//Ensure room display is current.
	this->pRoomWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame);
	this->pRoomWidget->DisplayPersistingImageOverlays(this->sCueEvents);

	//Get first command.
	this->currentCommandIter = CGameScreen::pCurrentGame->Commands.Get(this->pDemo->wBeginTurnNo);
	this->dwNextCommandTime = SDL_GetTicks() + FIRST_COMMAND_DELAY;

	this->dwSavedMoveDuration = this->pRoomWidget->GetMoveDuration();

	this->bBeforeFirstTurn = true;

	InitKeysymToCommandMap(g_pTheDB->GetCurrentPlayerSettings());

	return true;
}

//
//Private methods.
//

//*****************************************************************************
void CDemoScreen::OnDeactivate()
//On leaving the screen, clears any cue events left around so they won't show
//the next time a game screen (or derived class) is activated.
{
	ClearCueEvents();

	//Save updated persistent player info.
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (pCurrentPlayer)
	{
		CDbPackedVars& vars = pCurrentPlayer->Settings;
		vars.SetVar(Settings::MoveCounter, this->pRoomWidget->IsShowingMoveCount());
		vars.SetVar(Settings::PuzzleMode, this->pRoomWidget->IsShowingPuzzleMode());
		pCurrentPlayer->Update();
		delete pCurrentPlayer;
	}

	CGameScreen::OnDeactivate();
}

//*****************************************************************************
void CDemoScreen::OnKeyDown(
//Use the CScreen handler instead of CGameScreen's, which would process game
//commands in response to key pressed.
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key) //(in)
{
	CScreen::OnKeyDown(dwTagNo, Key);
	if (IsDeactivating())
		return;

	int nCommand = GetCommandForInputKey(BuildInputKey(Key));

	if (nCommand == CMD_ESCAPE) {
		Deactivate();
		return;
	}

	if (!this->bCanInteract)
		return;

	// The longer forward/backwards key is pressed the faster frames should pass
	UINT wMovesToDo = static_cast<UINT>(max(
		1,
		ceil(GetKeyRepeatDuration() / NAVIGATION_SPEED_INCREASE)
	));

	switch (nCommand) {
		case CMD_DEMO_SEEK_010:
			GoToTurn(0.1f);
			break;
		case CMD_DEMO_SEEK_020:
			GoToTurn(0.2f);
			break;
		case CMD_DEMO_SEEK_030:
			GoToTurn(0.3f);
			break;
		case CMD_DEMO_SEEK_040:
			GoToTurn(0.4f);
			break;
		case CMD_DEMO_SEEK_050:
			GoToTurn(0.5f);
			break;
		case CMD_DEMO_SEEK_060:
			GoToTurn(0.6f);
			break;
		case CMD_DEMO_SEEK_070:
			GoToTurn(0.7f);
			break;
		case CMD_DEMO_SEEK_080:
			GoToTurn(0.8f);
			break;
		case CMD_DEMO_SEEK_090:
			GoToTurn(0.9f);
			break;
		case CMD_DEMO_SEEK_100:
			GoToTurn(1.0f);
			break;
		case CMD_DEMO_GOTO:
			GoToTurn();
			break;
		case CMD_RESTART:
			GoToTurn(0U);
			break;
		case CMD_EXTRA_SHOW_HELP:
		{
			CHtmlDialogWidget *pHtmlDialogWidget = GetWidget<CHtmlDialogWidget>(TAG_HELPDIALOG);
			ASSERT(pHtmlDialogWidget);
			pHtmlDialogWidget->SetPageToLoad("standalone\\demoplayback.html");
			pHtmlDialogWidget->Display();
			RequestPaint();
		}
			break;
		case CMD_EXTRA_PUZZLE_MODE_OPTIONS:
			ShowPuzzleModeOptions();
			this->pRoomWidget->ShowPuzzleMode(true);
			break;
		case CMD_EXTRA_TOGGLE_PUZZLE_MODE:
			g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
			this->pRoomWidget->TogglePuzzleMode();
			this->pRoomWidget->DirtyRoom();
			break;
		case CMD_DEMO_SPEED_DOWN:
			this->bPaused = this->bPauseNextMove = false;
			if (this->fMoveRateMultiplier < 20.0f) this->fMoveRateMultiplier *= 1.1f;
			this->dwNextCommandTime = 0;	//play next move immediately
			break;
		case CMD_DEMO_SPEED_UP:
			this->bPaused = this->bPauseNextMove = false;
			if (this->fMoveRateMultiplier > 0.15f) this->fMoveRateMultiplier *= 0.9f;
			this->dwNextCommandTime = 0;	//play next move immediately
			break;
		case CMD_DEMO_PREV:
			if (this->GetCurrentTurn() < wMovesToDo)
				GoToTurn(0U);
			else
				GoToTurn(GetCurrentTurn() - wMovesToDo);
		break;
		case CMD_DEMO_NEXT:
			while (wMovesToDo--) {
				AdvanceTurn();
			}
			this->bPaused = true;
			this->bPauseNextMove = true;
			break;
		
		case CMD_EXTRA_TOGGLE_TURN_COUNT:
			this->pRoomWidget->ToggleMoveCount();
			break;

		case CMD_DEMO_PAUSE:
			if (this->dwNextCommandTime == static_cast<UINT>(-1))	//if watching move-by-move
				this->bPaused = false;	//resume normal playback speed
			else
				this->bPaused = !this->bPaused;

			if (this->bPaused)
			{
				this->pRoomWidget->SetMoveDuration(0);	//finish the latest move animation immediately
				RetainEffectCleanup(true); // to prevent subtitles and images from being cleared when room is redrawn
				DrawCurrentTurn();
				RetainEffectCleanup(false);
			}
			else {
				this->dwNextCommandTime = 0;	//play next move immediately
				if (this->dwSavedMoveDuration)
					this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration);
			}
			break;

		case CMD_DEMO_UNIFORM_SPEED:
			this->bUniformTurnSpeed = !this->bUniformTurnSpeed;
			break;
	}
}

//*****************************************************************************
void CDemoScreen::OnBetweenEvents()
//Called between events.
{
	if (this->bBeforeFirstTurn)
	{
		this->bBeforeFirstTurn = false;
		DrawCurrentTurn();
	}

	//Animate the game screen.
	CGameScreen::OnBetweenEvents();

	//Process next command if it's time.
	if (this->bPaused)
		return;
	Uint32 dwNow = SDL_GetTicks();
	if (dwNow >= this->dwNextCommandTime)
		this->AdvanceTurn();
}

//*****************************************************************************
void CDemoScreen::GoToTurn(const UINT wTargetTurnNo)
// Used only when can interact - seeks the demo position to the provided internal turn number
{
	if (!this->bCanInteract)
		return;

	const UINT wNewTurnNo = min(
		this->pDemo->wEndTurnNo, 
		max(this->pDemo->wBeginTurnNo, wTargetTurnNo)
	);

	if (wNewTurnNo == GetCurrentTurn())
		return;

	// If game is not active then it's possible some global state was changed and we need to restore things back to how they were initially
	// to avoid things like suddenly the room being conquered from the start
	if (!this->pCurrentGame->bIsGameActive && wNewTurnNo != 0)
		LoadDemoGame(this->pDemo->dwDemoID);

	if (wNewTurnNo == 0) {
		LoadDemoGame(this->pDemo->dwDemoID); // Just reset everything for simplicity

	} else if (wNewTurnNo < GetCurrentTurn()) {
		const bool bWasLightTurnedThisTurn = this->sCueEvents.HasOccurred(CID_LightToggled);
		const bool bWasLightTilesChangedThisTurn = this->sCueEvents.HasOccurred(CID_LightTilesChanged);

		//Move command sequence back one move.
		CGameScreen::pCurrentGame->Commands.Unfreeze();
		CGameScreen::pCurrentGame->SetTurn(wNewTurnNo, this->sCueEvents);
		CGameScreen::ClearSpeech();
		CGameScreen::pCurrentGame->Commands.Freeze();
		if (bWasLightTurnedThisTurn)
			this->sCueEvents.Add(CID_LightToggled); // Needed for the lights to be recalculated
		if (bWasLightTilesChangedThisTurn)
			this->sCueEvents.Add(CID_LightTilesChanged); // Needed for the lights to be recalculated

	} else if (wNewTurnNo == GetCurrentTurn() + 1) {
		AdvanceTurn();

	} else {
		CGameScreen::pCurrentGame->Commands.Unfreeze();
		CGameScreen::pCurrentGame->PlayCommandsToTurn(wNewTurnNo, this->sCueEvents);
		CGameScreen::pCurrentGame->Commands.Freeze();
	}


	this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetCurrent();

	DrawCurrentTurn();
	UpdateSign();
	this->dwNextCommandTime = static_cast<UINT>(-1);
}

//*****************************************************************************
void CDemoScreen::GoToTurn()
{
	WCHAR temp[10];
	WSTRING message = g_pTheDB->GetMessageText(MID_GotoDemoMovePrompt);
	message = WCSReplace(message, AsciiToUnicode("%end%"), _itoW(this->pDemo->wEndTurnNo - this->pDemo->wBeginTurnNo, temp, 10, 10));

	CScreen::TextInputDialogOptions options(
		message.c_str(),
		false, false, true
	);
	WSTRING wstrInput = _itoW(GetCurrentTurn() - this->pDemo->wBeginTurnNo, temp, 10, 10);
	const UINT dwAnswerTagNo = CScreen::ShowTextInputMessage(wstrInput, options);
	if (dwAnswerTagNo == TAG_OK)
	{
		const int turnNumber = _Wtoi(wstrInput.c_str());
		GoToTurn((UINT)turnNumber + this->pDemo->wBeginTurnNo);	
	}
}

//*****************************************************************************
void CDemoScreen::GoToTurn(const float fDemoPositionFraction)
{
	const UINT wNewTurn = static_cast<UINT>(
		this->pDemo->wBeginTurnNo
		+ (this->pDemo->wEndTurnNo - this->pDemo->wBeginTurnNo) * fDemoPositionFraction
	);
	GoToTurn(wNewTurn);
}

//*****************************************************************************
void CDemoScreen::AdvanceTurn()
{
	Uint32 dwNow = SDL_GetTicks();

	if (!this->pCurrentGame->bIsGameActive)
		return; // Don't allow going past player death

	if (IsAtDemoEnd())
		return; // The player is supposed to end the demo on their own

	// Don't show flashing messages for exit level/secret room/hold mastery on single-room victory demos
	bool bClearFlashingMessages = true;

	UINT wX=(UINT)-1, wY=(UINT)-1;
	int nCommand = static_cast<int>(this->currentCommandIter->bytCommand);
	if (CGameScreen::pCurrentGame->swordsman.wPlacingDoubleType &&
		nCommand != CMD_DOUBLE && !bIsAnswerCommand(nCommand))
	{
		//Upgrade 2.0 double placement move sequence with a single CMD_DOUBLE.
		CGameScreen::pCurrentGame->Commands.Unfreeze();
		CGameScreen::pCurrentGame->ReplaceDoubleCommands();
		CGameScreen::pCurrentGame->Commands.Freeze();
		this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetCurrent();
		if (this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end())
		{
			Deactivate(); //double placement never completed -- nothing else to show
			return;
		}
		nCommand = this->currentCommandIter->bytCommand;
	}

	if (bIsComplexCommand(nCommand))	//handle two-part commands here
	{
		vector<COMMANDNODE>::const_iterator data = this->currentCommandIter + 1;
		ASSERT(data != CGameScreen::pCurrentGame->Commands.end());
		wX = data->bytCommand;
		wY = data->byt10msElapsedSinceLast;
	}
	ProcessCommand(nCommand, wX, wY);

	//Check for last turn in demo.
	if (CGameScreen::pCurrentGame->wTurnNo > this->pDemo->wEndTurnNo)
	{
		this->currentCommandIter = CGameScreen::pCurrentGame->Commands.end();
	} else {
		//Get next turn.
		if (nCommand == CMD_CLONE) {
			//rewinding to a temporal split point will desync the commands iterator
			//so here we need to re-sync it
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.Get(CGameScreen::pCurrentGame->wTurnNo);

			//call this to unlink characters deleted during room reset caused by temporal split, will crash game randomly otherwise
			CGameScreen::ClearSpeech(false, true);

			if (this->sCueEvents.HasOccurred(CID_ActivatedTemporalSplit))
				DrawCurrentTurn();
		} else {
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetNext();
		}
	}
	
	// Must call after current command iterator is updated
	UpdateSign();

	if (this->bUniformTurnSpeed)
		this->dwNextCommandTime = dwNow + static_cast<UINT>(UNIFORM_STEP_DELAY *
				(this->bCanInteract ? this->fMoveRateMultiplier : 1.0f));
	else
		this->dwNextCommandTime = dwNow +
			((this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end()) ?
			LAST_COMMAND_DELAY :
			static_cast<UINT>(this->currentCommandIter->byt10msElapsedSinceLast * 10 *
					(this->bCanInteract ? this->fMoveRateMultiplier : 1.0f)));

	//If one room in a multi-room demo just ended, show room transition immediately.
	if (this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end() &&
			this->pDemo->dwNextDemoID) //Multi-room demo.
	{
		//Show transition to new room once it is loaded.
		UINT wExitOrientation = NO_ORIENTATION;
		const CAttachableWrapper<UINT> *pExitOrientation =
				DYN_CAST(const CAttachableWrapper<UINT>*, const CAttachableObject*,
				CGameScreen::sCueEvents.GetFirstPrivateData(CID_ExitRoomPending));
		if (pExitOrientation)
			wExitOrientation = static_cast<UINT>(*pExitOrientation);

		//Load next demo and get first command.
		if (LoadDemoGame(this->pDemo->dwNextDemoID))
		{
			//Load succeeded -- start playing it next frame.
			this->pRoomWidget->ShowRoomTransition(wExitOrientation, CGameScreen::sCueEvents);
			this->pMapWidget->DrawMapSurfaceFromRoom(CGameScreen::pCurrentGame->pRoom);
			this->pMapWidget->RequestPaint();
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetFirst();
			bClearFlashingMessages = false;
		}
	}

	//Move-by-move viewing.
	if (this->bPauseNextMove)
	{
		//Effectively pause the game by setting "infinite" wait to next move.
		//This is better then pausing the game since animation continues.
		this->dwNextCommandTime = static_cast<UINT>(-1);
		this->bPauseNextMove = false;
	}

	if (bClearFlashingMessages)
		this->pRoomWidget->RemoveLastLayerEffectsOfType(EFLASHTEXT);
}

//*****************************************************************************
int CDemoScreen::HandleEventsForPlayerDeath(CCueEvents &CueEvents)
{
	return CMD_WAIT;
}

//*****************************************************************************
bool CDemoScreen::IsAtDemoEnd()
{
	return this->pCurrentGame->wTurnNo > this->pDemo->wEndTurnNo;
}

//*****************************************************************************
void CDemoScreen::SetReplayOptions(const bool bCanInteract)
{
	this->bCanInteract = bCanInteract;
	this->fMoveRateMultiplier = 1.0f;
	if (!bCanInteract) this->bPaused = false;
	if (this->dwSavedMoveDuration)
		this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration);
	this->bPauseNextMove = false;
	if (!bCanInteract)
		this->bUniformTurnSpeed = false; //disable this setting when speed should not be modified
}

//*****************************************************************************
void CDemoScreen::SetSignTextToCurrentRoom()
{
	const SDL_Color Red = { 196, 0, 0, 0 };

	CGameScreen::SetSignTextToCurrentRoom();

	bool bAppendMove = true;
	this->signColor = Black;

	if (IsAtDemoEnd()) {
		this->wstrSignText = g_pTheDB->GetMessageText(MID_DemoEnded);
		this->signColor = Red;
		bAppendMove = false;

	} else if (!this->pCurrentGame->bIsGameActive) {
		this->wstrSignText = g_pTheDB->GetMessageText(MID_DemoEndedEarly);
		this->signColor = Red;
	}

	if (bAppendMove) {
		const UINT wMoveNow = CGameScreen::pCurrentGame->wTurnNo - this->pDemo->wBeginTurnNo;
		const UINT wMovesTotal = this->pDemo->wEndTurnNo - this->pDemo->wBeginTurnNo;

		WCHAR temp[10];
		WSTRING suffix = g_pTheDB->GetMessageText(MID_DemoMoveNumberSuffix);
		suffix = WCSReplace(suffix, AsciiToUnicode("%now%"), _itoW(wMoveNow, temp, 10, 10));
		suffix = WCSReplace(suffix, AsciiToUnicode("%total%"), _itoW(wMovesTotal, temp, 10, 10));

		this->wstrSignText += suffix;
	}

	PaintSign();
}

//*****************************************************************************
int CDemoScreen::GetCommandForInputKey(const InputKey inputKey) const
{
	switch (ReadInputKey(inputKey)) {
		case SDLK_ESCAPE: return CMD_ESCAPE;
		case SDLK_1: return CMD_DEMO_SEEK_010;
		case SDLK_2: return CMD_DEMO_SEEK_020;
		case SDLK_3: return CMD_DEMO_SEEK_030;
		case SDLK_4: return CMD_DEMO_SEEK_040;
		case SDLK_5: return CMD_DEMO_SEEK_050;
		case SDLK_6: return CMD_DEMO_SEEK_060;
		case SDLK_7: return CMD_DEMO_SEEK_070;
		case SDLK_8: return CMD_DEMO_SEEK_080;
		case SDLK_9: return CMD_DEMO_SEEK_090;
		case SDLK_0: return CMD_DEMO_SEEK_100;
	}

	const UINT nCommand = CRoomScreen::GetCommandForInputKey(inputKey);

	switch (nCommand) {
		case CMD_N: return CMD_DEMO_SPEED_UP;
		case CMD_S: return CMD_DEMO_SPEED_DOWN;
		case CMD_W: return CMD_DEMO_PREV;
		case CMD_E: return CMD_DEMO_NEXT;
		case CMD_WAIT: return CMD_DEMO_PAUSE;
		case CMD_NW: return CMD_DEMO_UNIFORM_SPEED;
		case CMD_EXTRA_STATS: return CMD_DEMO_GOTO;
	}

	switch (ReadInputKey(inputKey)) {
		case SDLK_KP_8: case SDLK_UP: return CMD_DEMO_SPEED_UP;
		case SDLK_KP_2: case SDLK_DOWN: return CMD_DEMO_SPEED_DOWN;
		case SDLK_KP_4: case SDLK_LEFT: return CMD_DEMO_PREV;
		case SDLK_KP_6: case SDLK_RIGHT: return CMD_DEMO_NEXT;
		case SDLK_KP_7: return CMD_DEMO_UNIFORM_SPEED;
		case SDLK_KP_5: case SDLK_SPACE: return CMD_DEMO_PAUSE;
	}
	return nCommand;
}