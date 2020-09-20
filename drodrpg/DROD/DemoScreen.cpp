// $Id: DemoScreen.cpp 9354 2009-03-22 15:10:55Z mrimer $

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

#include "DemoScreen.h"
#include "DrodScreenManager.h"
#include "MapWidget.h"
#include "RoomWidget.h"
#include "../DRODLib/CueEvents.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbCommands.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

const UINT FIRST_COMMAND_DELAY = 500;
const UINT LAST_COMMAND_DELAY = 500;
const UINT UNIFORM_STEP_DELAY = 1000/15; //15 tps

float CDemoScreen::fMoveRateMultiplier = 1.0;

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
//	CGameScreen::pCurrentGame->SetAutoSaveOptions(ASO_NONE);

	//Rewind current game to beginning of demo.
	CGameScreen::pCurrentGame->SetTurn(this->pDemo->wBeginTurnNo, this->sCueEvents);

	//Update game screen widgets for new room and current game.
	if (!CGameScreen::pMapWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame) ||
			!CGameScreen::pRoomWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame)) 
		return false;

	//Don't modify any commands during demo playback.
	CGameScreen::pCurrentGame->Commands.Freeze();

/*
	//Since there is no way to flag hold mastery in a demo,
	//assume the player can go through master walls in demos.
	//This is a safe assumption to make: if it's wrong, the player wouldn't
	//be making any moves to go through master walls anyway (observe that
	//bumps against intact master walls are not allowed in CCurrentGame::ProcessPlayer.)
	CGameScreen::pCurrentGame->bHoldMastered = true;
*/
	CGameScreen::SetGameAmbience();
	CGameScreen::pRoomWidget->ClearEffects();
	CGameScreen::SetSignTextToCurrentRoom();
	CGameScreen::SetMusicStyle();
	CGameScreen::ClearSpeech();
	
	return true;
}

//
//CDemoScreen protected methods.
//

//*****************************************************************************
CDemoScreen::CDemoScreen()
	: CGameScreen(SCR_Demo)
	, dwNextCommandTime(0L)
	, pDemo(NULL), bCanChangeSpeed(false)
	, bPaused(false), bPauseNextMove(false)
	, dwSavedMoveDuration(0)
	, bUniformTurnSpeed(false)
//Constructor.
{
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
	
	PaintClock(true);

	//Set frame rate as high as needed for smooth animations.
	SetBetweenEventsInterval(12);

	//Reset vars.
	this->bPaused = this->bPauseNextMove = false;
	this->pRoomWidget->ShowPlayer();

	//Ensure room display is current.
	this->pRoomWidget->LoadFromCurrentGame(CGameScreen::pCurrentGame);

	//Get first command.
	this->currentCommandIter = 
			CGameScreen::pCurrentGame->Commands.Get(this->pDemo->wBeginTurnNo);
	this->dwNextCommandTime = SDL_GetTicks() + FIRST_COMMAND_DELAY;

	this->dwSavedMoveDuration = this->pRoomWidget->GetMoveDuration();

	this->bBeforeFirstTurn = true;

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

	//Hitting a key ends the demo.
	switch (Key.keysym.sym)
	{
		//Change speed.
		case SDLK_KP2: case SDLK_DOWN:
			this->bPaused = this->bPauseNextMove = false;
			if (this->fMoveRateMultiplier < 20.0f) this->fMoveRateMultiplier *= 1.1f;
			this->dwNextCommandTime = 0;	//play next move immediately
			break;
		case SDLK_KP8: case SDLK_UP:
			this->bPaused = this->bPauseNextMove = false;
			if (this->fMoveRateMultiplier > 0.15f) this->fMoveRateMultiplier *= 0.9f;
			this->dwNextCommandTime = 0;	//play next move immediately
			break;

		//Pause.
		case SDLK_SPACE: case SDLK_KP5:
			if (!this->bCanChangeSpeed) break;
			if (this->dwNextCommandTime == static_cast<UINT>(-1))	//if watching move-by-move
				this->bPaused = false;	//resume normal playback speed
			else
				this->bPaused = !this->bPaused;
			if (this->bPaused)
			{
				this->pRoomWidget->SetMoveDuration(0);	//finish the latest move animation immediately
				RetainSubtitleCleanup(true); // to prevent subtitle texts being cleared when room is redrawn
				DrawCurrentTurn();
				RetainSubtitleCleanup();
			} else {
				this->dwNextCommandTime = 0;	//play next move immediately
				if (this->dwSavedMoveDuration)
					this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration);
			}
		break;

		//Back one move.
		case SDLK_KP4: case SDLK_LEFT:
			if (!this->bCanChangeSpeed) break;
			if (CGameScreen::pCurrentGame->wTurnNo <= this->pDemo->wBeginTurnNo)
				break; //can't go before beginning of demo

			//Move command sequence back one move.
			CGameScreen::pCurrentGame->Commands.Unfreeze();
			CGameScreen::pCurrentGame->SetTurn(CGameScreen::pCurrentGame->wTurnNo - 1, this->sCueEvents);
			CGameScreen::ClearSpeech();
			CGameScreen::pCurrentGame->Commands.Freeze();
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.Get(CGameScreen::pCurrentGame->wTurnNo);
			DrawCurrentTurn();

			this->bPaused = false;
			this->bPauseNextMove = true;
		break;

		//Forward one move.
		case SDLK_KP6: case SDLK_RIGHT:
			if (!this->bCanChangeSpeed) break;
			this->bPaused = false;
			this->bPauseNextMove = true;
			this->dwNextCommandTime = 0;	//play next move immediately
			this->pRoomWidget->SetMoveDuration(0);	//draw move immediately
		break;

		//Toggle uniform turn playback speed.
		case SDLK_KP7:
			if (!this->bCanChangeSpeed) break;
			this->bUniformTurnSpeed = !this->bUniformTurnSpeed;
		break;

		//Toggle move count display.
		case SDLK_F7:
			if (!this->bCanChangeSpeed) break;
			this->pRoomWidget->ToggleMoveCount();
		break;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (Key.keysym.mod & KMOD_ALT && !GetHotkeyTag(Key.keysym))
				//going to next case
		case SDLK_F10:
		case SDLK_TAB:
		case SDLK_LALT: case SDLK_RALT:
			//don't do anything here
			break;
		default:
			Deactivate();
			break;
	}
}

//******************************************************************************
void CDemoScreen::OnMouseUp(
//Handling mouse clicks.
//
//Params:
	const UINT /*dwTagNo*/,   const SDL_MouseButtonEvent &/*Button*/)
{
	//Mouse click ends the demo.
	Deactivate();
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
	{
		//End of demo.
		if (this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end())
		{
			Deactivate();
			return;
		}

		UINT wX=(UINT)-1, wY=(UINT)-1;
		const int nCommand = static_cast<int>(this->currentCommandIter->command);
		if (bIsComplexCommand(nCommand))	//handle two-part commands here
		{
			vector<COMMANDNODE>::const_iterator data = this->currentCommandIter + 1;
			ASSERT(data != CGameScreen::pCurrentGame->Commands.end());
			wX = data->command;
			wY = data->msElapsedSinceLast;
		}
		ProcessCommand(nCommand, wX, wY);
		
		//If a move was made that places the game in a state when no more turns
		//are allowed in the room, stop replaying this demo.
		//Events causing this could involve player death or exiting the room.
		//Note these events could occur when replaying a corrupted demo that ends
		//prematurely due to changed room state or game logic.
		if (!this->pCurrentGame->bIsGameActive)
		{
			Deactivate();
			return;
		}

		//Check for last turn in demo.
		if ((int)CGameScreen::pCurrentGame->wTurnNo - 1 >= (int)this->pDemo->wEndTurnNo)
		{
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.end();
		}
		else  //Get next turn.
		{
			this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetNext();
		}

		if (this->bUniformTurnSpeed)
			this->dwNextCommandTime = dwNow + static_cast<UINT>(UNIFORM_STEP_DELAY *
					(this->bCanChangeSpeed ? this->fMoveRateMultiplier : 1.0f));
		else
			this->dwNextCommandTime = dwNow +
					((this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end()) ?
					LAST_COMMAND_DELAY :
					static_cast<UINT>(this->currentCommandIter->msElapsedSinceLast *
							(this->bCanChangeSpeed ? this->fMoveRateMultiplier : 1.0f)));

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
				this->pRoomWidget->ShowRoomTransition(wExitOrientation);
				this->pMapWidget->DrawMapSurfaceFromRoom(CGameScreen::pCurrentGame->pRoom);
				this->pMapWidget->RequestPaint();
				this->currentCommandIter = CGameScreen::pCurrentGame->Commands.GetFirst();
			} //else load failed -- demo stops next frame
		}

		//Move-by-move viewing.
		if (this->bPauseNextMove)
		{
			//Effectively pause the game by setting "infinite" wait to next move.
			//This is better then pausing the game since animation continues.
			this->dwNextCommandTime = static_cast<UINT>(-1);
			this->bPauseNextMove = false;
		}

		//If watching a victory demo, immediately stop once the room has been left.
		if (this->currentCommandIter == CGameScreen::pCurrentGame->Commands.end() &&
				CGameScreen::sCueEvents.HasOccurred(CID_ExitRoomPending))
			Deactivate();
	}
}

//*****************************************************************************
void CDemoScreen::SetReplayOptions(const bool bChangeSpeed)
{
	this->bCanChangeSpeed = bChangeSpeed;
	this->fMoveRateMultiplier = 1.0f;
	if (!bChangeSpeed) this->bPaused = false;
	if (this->dwSavedMoveDuration)
		this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration);
	this->bPauseNextMove = false;
	if (!bChangeSpeed)
		this->bUniformTurnSpeed = false; //disable this setting when speed should not be modified
}

