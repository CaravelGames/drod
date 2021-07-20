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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

/*
A poor man's guide to how Image Overlay effects work.

In its simplest, an Image Overlay is a queue of commands that each time 0 or more milliseconds
and are executed one after the other. This is the core functionality, but besides it you have:
 - Looping  - essentially rerun the whole queue X number of times
 - Parallel commands - they run parallel to other commands, all at once
 - Cancelling layers - handled externally
*/

#include "ImageOverlayEffect.h"
#include "DrodBitmapManager.h"
#include "DrodEffect.h"
#include "RoomWidget.h"
#include "../DRODLib/CharacterCommand.h"
#include "../DRODLib/CurrentGame.h"

#include <FrontEndLib/Screen.h>

#include <map>
#include <string>
using std::map;
using std::string;

const int ORIGINAL_SCALE = 100; //percent

long nextDrawSequence = 0; //each new effect is drawn after/on top of the previous ones

int clipAlpha(int val)
{
	if (val < 0)
		return 0;
	if (val > 255)
		return 255;
	return val;
}

int clipAngle(int val)
{
	if (val < 0) {
		const int cycles = -val/360;
		return val + (cycles+1) * 360;
	}
	return val % 360;
}

int clipScale(int val)
{
	if (val <= 0)
		return 1;

	return val;
}

//*****************************************************************************
CImageOverlayEffect::CImageOverlayEffect(
	CWidget *pSetWidget,
	const CImageOverlay *pImageOverlay,
	const UINT turnNo,
	const Uint32 dwStartTime)
	: CEffect(pSetWidget, (UINT)-1, EIMAGEOVERLAY)
	, drawSequence(++nextDrawSequence)
	, pImageSurface(NULL), pAlteredSurface(NULL)
	, bPrepareAlteredImage(false)
	, x(0), y(0)
	, drawX(0), drawY(0)
	, alpha(255), drawAlpha(255)
	, angle(0), scale(ORIGINAL_SCALE)
	, jitter(0)
	, index(UINT(-1))
	, loopIteration(0), maxLoops(0)
	, startOfNextEffect(dwStartTime)
	, pRoomWidget(NULL)
	, turnNo(turnNo)
	, instanceID(0)
	, drawSourceRect(MAKE_SDL_RECT(0, 0, 0, 0))
	, drawDestinationRect(MAKE_SDL_RECT(0, 0, 0, 0))
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	ASSERT(pImageOverlay);

	this->pImageSurface = g_pTheDBM->LoadImageSurface(pImageOverlay->imageID);
	static const SDL_Rect rect = {0,0,0,0};
	this->dirtyRects.push_back(rect);

	InitParams();

	this->commands = pImageOverlay->commands;
	this->instanceID = pImageOverlay->instanceID;
}

CImageOverlayEffect::~CImageOverlayEffect()
{
	if (this->pImageSurface)
		SDL_FreeSurface(this->pImageSurface);
	if (this->pAlteredSurface)
		SDL_FreeSurface(this->pAlteredSurface);
}

void CImageOverlayEffect::InitParams()
{
	this->x = 0;
	this->y = 0;
	this->alpha = 255;
	this->drawX = 0;
	this->drawY = 0;
	this->drawAlpha = 255;
	this->angle = 0;
	this->jitter = 0;
	this->scale = ORIGINAL_SCALE;

	static const SDL_Rect rect = {0,0,0,0};
	this->sourceClipRect = rect;
	this->parallelCommands.clear();
}

int CImageOverlayEffect::getGroup() const
{
	for (ImageOverlayCommands::const_iterator it = commands.begin();
		it != commands.end(); ++it)
	{
		const ImageOverlayCommand& c = *it;
		if (c.type == ImageOverlayCommand::Group)
			return c.val[0];
	}

	return ImageOverlayCommand::DEFAULT_GROUP;
}

bool CImageOverlayEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (!this->pImageSurface)
		return false;

	if (!AdvanceState(wDeltaTime))
		return false;

	if (this->bPrepareAlteredImage) {
		PrepareAlteredImage();
		this->bPrepareAlteredImage = false;
	}

	PrepareDrawProperties();

	return true;
}

void CImageOverlayEffect::PrepareDrawProperties()
{
	this->drawX = this->x;
	this->drawY = this->y;

	if (this->jitter) {
		this->drawX += ROUND(fRAND_MID(this->jitter));
		this->drawY += ROUND(fRAND_MID(this->jitter));
	}

	this->pOwnerWidget->GetRect(this->drawDestinationRect);

	this->drawX = this->x;
	this->drawY = this->y;
	this->drawSourceRect = this->sourceClipRect;

	if (this->pAlteredSurface) {
		SDL_Surface* pSrcSurface = this->pAlteredSurface;
		if (!this->drawSourceRect.w)
			this->drawSourceRect.w = pSrcSurface->w;
		else if (this->drawSourceRect.w > pSrcSurface->w)
			this->drawSourceRect.w = pSrcSurface->w;
		if (!this->drawSourceRect.h)
			this->drawSourceRect.h = pSrcSurface->h;
		else if (this->drawSourceRect.h > pSrcSurface->h)
			this->drawSourceRect.h = pSrcSurface->h;
		this->drawX += (int(this->pImageSurface->w) - pSrcSurface->w) / 2; //keep centered
		this->drawY += (int(this->pImageSurface->h) - pSrcSurface->h) / 2; //keep centered
	}
	else {
		SDL_Surface* pSrcSurface = this->pImageSurface;
		if (!this->drawSourceRect.w)
			this->drawSourceRect.w = this->pImageSurface->w;
		if (!this->drawSourceRect.h)
			this->drawSourceRect.h = this->pImageSurface->h;
	}

	this->drawAlpha = Uint8(this->alpha * this->fOpacity);

	if (IsImageDrawn())
		this->dirtyRects[0] = this->drawDestinationRect;
	else {
		this->dirtyRects[0].w = this->dirtyRects[0].h = 0;
		this->drawDestinationRect.w = 0;
		this->drawDestinationRect.h = 0;
	}
}

void CImageOverlayEffect::Draw(SDL_Surface& destSurface)
{
	SDL_Surface* pSrcSurface;

	if (this->pAlteredSurface)
		pSrcSurface = this->pAlteredSurface;
	else
		pSrcSurface = this->pImageSurface;

	if (this->drawDestinationRect.w > 0 && this->drawDestinationRect.h > 0) {
		const bool bSurfaceAlpha = !this->pImageSurface->format->Amask && this->alpha < 255;
		if (bSurfaceAlpha) {
			EnableSurfaceBlending(pSrcSurface, this->drawAlpha);
			SDL_BlitSurface(pSrcSurface, &this->drawSourceRect, &destSurface, &this->drawDestinationRect);
			DisableSurfaceBlending(pSrcSurface);
		}
		else {
			g_pTheBM->BlitAlphaSurfaceWithTransparency(this->drawSourceRect, pSrcSurface, this->drawDestinationRect, &destSurface, this->drawAlpha);
		}
	}
}

bool CImageOverlayEffect::IsImageDrawn()
{
	return this->drawAlpha > 0 && PositionDisplayInsideRect(this->drawSourceRect, this->drawDestinationRect, this->drawX, this->drawY);
}

bool CImageOverlayEffect::AdvanceState(const UINT wDeltaTime)
{
	// We execute commands one by one and use up the time we have allocated for this rendering pass
	Uint32 dwRemainingTime = wDeltaTime;
	// For infinite loop protection
	const UINT wInitialIndex = this->index;

	if (this->commands.size() == 0)
		return false; // If there are no commands to run then just finish the effect

	if (this->index == (UINT)-1) {
		++this->index;
		StartNextCommand();
	}

	// do{} is used because on the first draw wDeltaTime will be 0, and we still want non time-based commands to happen
	do {
		Uint32 dwConsumedTime = UpdateCommand(this->commands[this->index], this->executionState, dwRemainingTime);
		UpdateParallelCommands(dwConsumedTime);

		ASSERT(dwConsumedTime <= dwRemainingTime);
		dwRemainingTime -= dwConsumedTime;

		if (IsCurrentCommandFinished()) {
			FinishCommand(this->commands[this->index], this->executionState);
			++this->index;
			StartNextCommand();
		}
		else
			break;
	} while (CanContinuePlayingEffect(dwRemainingTime));

	if (dwRemainingTime > 0)
		dwRemainingTime -= UpdateParallelCommands(dwRemainingTime);

	if (dwRemainingTime > 0 && IsCommandQueueFinished()) {
		if (wInitialIndex == (UINT)-1 && dwRemainingTime == wDeltaTime)
			return false; // Infinite loop protection

		++this->loopIteration;
		if (!CanLoop())
			return false;

		//Loop
		InitParams();
		this->index = (UINT)-1;
		AdvanceState(dwRemainingTime);
	}

	return CanLoop() || !IsCommandQueueFinished();
}

bool CImageOverlayEffect::CanLoop() const
{
	return this->maxLoops == ImageOverlayCommand::NO_LOOP_MAX || this->loopIteration < this->maxLoops;
}

bool CImageOverlayEffect::CanContinuePlayingEffect(const Uint32 dwRemainingTIme) const
{
	// There are no more effects to play, so AdvanceState() has to handle looping nopw
	if (IsCommandQueueFinished())
		return false;

	// If there is any remaining time then continue playing commands
	if (dwRemainingTIme > 0)
		return true;

	// If it's a time based command we need to stop
	return !IsTimeBasedCommand(this->commands[this->index].type);
}

bool CImageOverlayEffect::IsCommandQueueFinished() const
{
	return this->index >= this->commands.size();
}

bool CImageOverlayEffect::IsCurrentCommandFinished() const
{
	if (IsCommandQueueFinished())
		return true;
	
	const ImageOverlayCommand command = this->commands[this->index];
	if (IsTimeBasedCommand(command.type))
		return this->executionState.remainingTime == 0;

	else if (IsTurnBasedCommand(command.type)) {
		const CDbRoom* pRoom = this->pRoomWidget->GetRoom();
		const UINT gameTurn = pRoom ? pRoom->GetCurrentGame()->wPlayerTurn : 0;

		return this->executionState.endTurn == gameTurn;
	}
	else
		return true;
}

Uint32 CImageOverlayEffect::UpdateCommand(
	const ImageOverlayCommand& command,     //(in) Command to be updated
	CommandExecution& executionState,       //(in) Command's execution state
	const Uint32 wRemainingTime)            //(in) Amount of time that the command can consume
// Updates the consumed time in the execution state and updates the display state of the image to match the command
// Returns: amount of milliseconds consumed by the effect
{
	// Non-time-based commands do nothing here
	if (!IsTimeBasedCommand(command.type))
		return 0;

	// Parallel commands executed
	const Uint32 dwConsumedTime = min(executionState.remainingTime, wRemainingTime);
	executionState.remainingTime -= dwConsumedTime;

	const float t = 1.0f - (float(executionState.remainingTime) / float(executionState.duration));
	switch (command.type) {
		case ImageOverlayCommand::FadeToAlpha:
		{
			const int deltaAlpha = command.val[0] - executionState.startAlpha;
			this->alpha = clipAlpha(executionState.startAlpha + int(deltaAlpha * t));
		}
		break;
		case ImageOverlayCommand::Grow:
		{
			const int deltaScale = command.val[0];
			this->scale = clipScale(executionState.startScale + int(deltaScale * t));
			this->bPrepareAlteredImage = true;
		}
		break;
		case ImageOverlayCommand::Jitter:
			//nothing to do here
			break;
		case ImageOverlayCommand::Move:
		{
			const int deltaX = command.val[0];
			const int deltaY = command.val[1];
			this->x = executionState.startX + int(deltaX * t);
			this->y = executionState.startY + int(deltaY * t);
		}
		break;
		case ImageOverlayCommand::MoveTo:
		{
			const int deltaX = command.val[0] - executionState.startX;
			const int deltaY = command.val[1] - executionState.startY;
			this->x = executionState.startX + int(deltaX * t);
			this->y = executionState.startY + int(deltaY * t);
		}
		break;
		case ImageOverlayCommand::Rotate:
		{
			const int deltaAngle = command.val[0];
			this->angle = clipAngle(executionState.startAngle + int(deltaAngle * t));
			this->bPrepareAlteredImage = true;
		}
		break;
		default: break;
	}

	return dwConsumedTime;
}


Uint32 CImageOverlayEffect::UpdateParallelCommands(const Uint32 dwDeltaTime)
// Updates the parallel commands, removing them when finished
// Returns: the max number of milliseconds consumed by any parallel commands
{
	if (this->parallelCommands.empty())
		return 0;

	vector<ParallelCommand> continuingCommands;
	Uint32 maxConsumedTime = 0;

	for (vector<ParallelCommand>::iterator it = this->parallelCommands.begin();
		it != this->parallelCommands.end(); ++it)
	{
		ParallelCommand& c = *it;
		const Uint32 dwConsumedTime = UpdateCommand(c.command, c.executionState, dwDeltaTime);

		if (dwConsumedTime > maxConsumedTime)
			maxConsumedTime = max(maxConsumedTime, dwConsumedTime);

		if (c.executionState.remainingTime == 0) {
			FinishCommand(c.command, c.executionState);
		}
		else
			continuingCommands.push_back(c);
	}

	this->parallelCommands = continuingCommands;

	return maxConsumedTime;
}


void CImageOverlayEffect::StartNextCommand()
{
	this->executionState.duration = this->startOfNextEffect;
	this->executionState.remainingTime = UINT(-1);
	this->executionState.endTurn = UINT(-1);

	if (IsCommandQueueFinished())
		return;

	const ImageOverlayCommand command = this->commands[this->index];

	const ImageOverlayCommand::IOC curCommand = command.type;
	int val = command.val[0];
	switch (curCommand) {
	case ImageOverlayCommand::CancelAll:
	case ImageOverlayCommand::CancelGroup:
	case ImageOverlayCommand::CancelLayer:
	case ImageOverlayCommand::Group:
	case ImageOverlayCommand::Layer:
		// Do nothing, these are handled externally
		return;

	case ImageOverlayCommand::AddX:
		this->x += val;
		break;
	case ImageOverlayCommand::AddY:
		this->y += val;
		break;

	case ImageOverlayCommand::Center:
		this->x = (int(this->pRoomWidget->GetW()) - int(this->pImageSurface->w)) / 2;
		this->y = (int(this->pRoomWidget->GetH()) - int(this->pImageSurface->h)) / 2;
		break;

	case ImageOverlayCommand::DisplayDuration:
		this->executionState.remainingTime = this->executionState.duration = max(0, val);
		break;

	case ImageOverlayCommand::DisplayRect:
		this->sourceClipRect.x = max(0, val);
		this->sourceClipRect.y = max(0, command.val[1]);
		this->sourceClipRect.w = min(max(0, command.val[2]), this->pImageSurface->w);
		this->sourceClipRect.h = min(max(0, command.val[3]), this->pImageSurface->h);
		break;

	case ImageOverlayCommand::DisplayRectModify:
		this->sourceClipRect.x = max(0, this->sourceClipRect.x + val);
		this->sourceClipRect.y = max(0, this->sourceClipRect.y + command.val[1]);
		this->sourceClipRect.w = min(max(0, this->sourceClipRect.w + command.val[2]), this->pImageSurface->w);
		this->sourceClipRect.h = min(max(0, this->sourceClipRect.h + command.val[3]), this->pImageSurface->h);

	case ImageOverlayCommand::DisplaySize:
		this->sourceClipRect.w = min(max(0, val), this->pImageSurface->w);
		this->sourceClipRect.h = min(max(0, command.val[1]), this->pImageSurface->h);
		break;

	case ImageOverlayCommand::FadeToAlpha:
		this->executionState.remainingTime = this->executionState.duration = max(0, command.val[1]);
		this->executionState.startAlpha = this->alpha;
		break;

	case ImageOverlayCommand::ParallelFadeToAlpha:
	{
		ParallelCommand c(command, this->executionState);
		c.command.type = ImageOverlayCommand::FadeToAlpha;
		c.executionState.duration = max(0, command.val[1]);
		c.executionState.remainingTime = max(0, command.val[1]);
		c.executionState.startAlpha = this->alpha;
		this->parallelCommands.push_back(c);
	}
	break;

	case ImageOverlayCommand::Grow:
		this->executionState.startScale = this->scale;
		this->executionState.remainingTime = this->executionState.duration = max(0, command.val[1]);
		break;

	case ImageOverlayCommand::ParallelGrow:
	{
		ParallelCommand c(command, this->executionState);
		c.command.type = ImageOverlayCommand::Grow;
		c.executionState.startScale = this->scale;
		c.executionState.duration = max(0, command.val[1]);
		c.executionState.remainingTime = max(0, command.val[1]);
		this->parallelCommands.push_back(c);
	}
	break;

	case ImageOverlayCommand::Jitter:
		this->jitter = max(0, val);
		this->executionState.remainingTime = this->executionState.duration = max(0, command.val[1]);
		break;

	case ImageOverlayCommand::ParallelJitter:
	{
		this->jitter = max(0, val);

		ParallelCommand c(command, this->executionState);
		c.command.type = ImageOverlayCommand::Jitter;
		c.executionState.duration = max(0, command.val[1]);
		c.executionState.remainingTime = max(0, command.val[1]);
		this->parallelCommands.push_back(c);
	}
	break;

		break;

	case ImageOverlayCommand::Loop:
		this->maxLoops = val;
		break;

	case ImageOverlayCommand::Move:
	case ImageOverlayCommand::MoveTo:
		this->executionState.startX = this->x;
		this->executionState.startY = this->y;
		this->executionState.remainingTime = this->executionState.duration = max(0, command.val[2]);
		break;
	case ImageOverlayCommand::ParallelMove:
	case ImageOverlayCommand::ParallelMoveTo:
	{
		ParallelCommand c(command, this->executionState);
		c.command.type = c.command.type == ImageOverlayCommand::ParallelMove
			? ImageOverlayCommand::Move
			: ImageOverlayCommand::MoveTo;
		c.executionState.startX = this->x;
		c.executionState.startY = this->y;
		c.executionState.duration = max(0, command.val[1]);
		c.executionState.remainingTime = max(0, command.val[1]);
		this->parallelCommands.push_back(c);
	}
	break;

	case ImageOverlayCommand::Rotate:
		this->executionState.startAngle = this->angle;
		this->executionState.remainingTime = this->executionState.duration = max(0, command.val[1]);
		break;

	case ImageOverlayCommand::ParallelRotate:
	{
		ParallelCommand c(command, this->executionState);
		c.command.type = ImageOverlayCommand::Rotate;
		c.executionState.startAngle = this->angle;
		c.executionState.duration = max(0, command.val[1]);
		c.executionState.remainingTime = max(0, command.val[1]);
		this->parallelCommands.push_back(c);
	}
	break;

	case ImageOverlayCommand::Scale:
		this->scale = clipScale(val);
		this->bPrepareAlteredImage = true;
		break;
	case ImageOverlayCommand::SetAlpha:
		this->alpha = clipAlpha(val);
		break;
	case ImageOverlayCommand::SetAngle:
		this->angle = clipAngle(val);
		this->bPrepareAlteredImage = true;
		break;
	case ImageOverlayCommand::SetX:
		this->x = val;
		break;
	case ImageOverlayCommand::SetY:
		this->y = val;
		break;
	case ImageOverlayCommand::SrcXY:
		this->sourceClipRect.x = max(0, val);
		this->sourceClipRect.y = max(0, command.val[1]);
		break;

	case ImageOverlayCommand::TurnDuration:
	{
		const CDbRoom* pRoom = this->pRoomWidget->GetRoom();
		const UINT gameTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;
		this->executionState.endTurn = gameTurn + max(0, val);
	}
	break;
	}
}

void CImageOverlayEffect::FinishCommand(
// Finalizes state update of a given command
	const ImageOverlayCommand& command,
	const CommandExecution& executionState)
{
	switch (command.type) {
	case ImageOverlayCommand::FadeToAlpha:
		this->alpha = clipAlpha(command.val[0]);
		break;
	case ImageOverlayCommand::Grow:
		this->scale = clipScale(executionState.startScale + command.val[0]);
		this->bPrepareAlteredImage = true;
		break;
	case ImageOverlayCommand::Jitter:
		this->jitter = 0;
		break;
	case ImageOverlayCommand::Move:
		this->x = executionState.startX + command.val[0];
		this->y = executionState.startY + command.val[1];
		break;
	case ImageOverlayCommand::MoveTo:
		this->x = command.val[0];
		this->y = command.val[1];
		break;
	case ImageOverlayCommand::Rotate:
		this->angle = clipAngle(executionState.startAngle + command.val[0]);
		this->bPrepareAlteredImage = true;
		break;
	case ImageOverlayCommand::TurnDuration:
		// This is required for any time-based command that runs afterwards to correctly
		// calculate the start time
		this->startOfNextEffect = CScreen::dwCurrentTicks;
		break;
	case ImageOverlayCommand::ParallelFadeToAlpha:
	case ImageOverlayCommand::ParallelRotate:
	case ImageOverlayCommand::ParallelMoveTo:
	case ImageOverlayCommand::ParallelMove:
	case ImageOverlayCommand::ParallelJitter:
	case ImageOverlayCommand::ParallelGrow:
	default: break;
	}
}

bool CImageOverlayEffect::PositionDisplayInsideRect(
	SDL_Rect& src, SDL_Rect& dest,
	int display_x, int display_y)
const
{
	if (display_x >= 0) {
		dest.x += display_x;

		if (display_x >= dest.w)
			return false;
		dest.w -= display_x;
	} else {
		const int delta = -display_x;
		src.x = delta;

		if (src.w <= delta)
			return false;
		src.w -= delta;
	}
	if (src.w > dest.w)
		src.w = dest.w;
	else if (dest.w > src.w)
		dest.w = src.w;

	if (display_y >= 0) {
		dest.y += display_y;

		if (display_y >= dest.h)
			return false;
		dest.h -= display_y;
	} else {
		const int delta = -display_y;
		src.y = delta;

		if (src.h <= delta)
			return false;
		src.h -= delta;
	}
	if (src.h > dest.h)
		src.h = dest.h;
	else if (dest.h > src.h)
		dest.h = src.h;

	return true;
}

void CImageOverlayEffect::PrepareAlteredImage()
{
	if (this->pAlteredSurface) {
		SDL_FreeSurface(this->pAlteredSurface);
		this->pAlteredSurface = NULL;
	}

	if (this->angle || this->scale != ORIGINAL_SCALE) {
		if (this->scale != ORIGINAL_SCALE) {
			const SDL_Surface *pSurface = this->pImageSurface;
			const Uint8 *pSrcPixel = (Uint8*)pSurface->pixels;
			const int new_width = pSurface->w * this->scale / 100;
			const int new_height = pSurface->h * this->scale / 100;
			this->pAlteredSurface = g_pTheDBM->ScaleSurface(pSurface, pSrcPixel,
				pSurface->w, pSurface->h, new_width, new_height);
		}

		if (this->angle) {
			SDL_Surface *pSurface = this->pAlteredSurface ? this->pAlteredSurface : this->pImageSurface;
			Uint8 *pSrcPixel = (Uint8*)pSurface->pixels;
			SDL_Surface *pRotatedSurface = g_pTheDBM->RotateSurface(pSurface,
					pSrcPixel, pSurface->w, pSurface->h, float(this->angle));
			if (pRotatedSurface) {
				if (this->pAlteredSurface)
					SDL_FreeSurface(this->pAlteredSurface);
				this->pAlteredSurface = pRotatedSurface;
			}
		}
	}
}

bool CImageOverlayEffect::IsTimeBasedCommand(const ImageOverlayCommand::IOC commandType) const {
	switch (commandType) {
		// Intentionally excluded parallel commands, see IsInstantCommand()
		case ImageOverlayCommand::DisplayDuration:
		case ImageOverlayCommand::FadeToAlpha:
		case ImageOverlayCommand::Grow:
		case ImageOverlayCommand::Jitter:
		case ImageOverlayCommand::Move:
		case ImageOverlayCommand::MoveTo:
		case ImageOverlayCommand::Rotate:
			return true;
		default:
			return false;
	}
}

bool CImageOverlayEffect::IsTurnBasedCommand(const ImageOverlayCommand::IOC commandType) const {
	return commandType == ImageOverlayCommand::TurnDuration;
}

bool CImageOverlayEffect::IsInstantCommand(const ImageOverlayCommand::IOC commandType) const {
	switch (commandType) {
		case ImageOverlayCommand::AddX:
		case ImageOverlayCommand::AddY:
		case ImageOverlayCommand::CancelAll:
		case ImageOverlayCommand::CancelGroup:
		case ImageOverlayCommand::CancelLayer:
		case ImageOverlayCommand::Center:
		case ImageOverlayCommand::DisplayRect:
		case ImageOverlayCommand::DisplayRectModify:
		case ImageOverlayCommand::DisplaySize:
		case ImageOverlayCommand::Rotate:
		case ImageOverlayCommand::Scale:
		case ImageOverlayCommand::SetAlpha:
		case ImageOverlayCommand::SetAngle:
		case ImageOverlayCommand::SetX:
		case ImageOverlayCommand::SetY:
		case ImageOverlayCommand::SrcXY:
		// Parallel commands may take time but they run parallelly so for the purpose of occupying
		// effect's time they are instant
		case ImageOverlayCommand::ParallelFadeToAlpha:
		case ImageOverlayCommand::ParallelJitter:
		case ImageOverlayCommand::ParallelMove:
		case ImageOverlayCommand::ParallelMoveTo:
		case ImageOverlayCommand::ParallelRotate:
		case ImageOverlayCommand::Invalid:
			return true;
		default:
			return false;
	}
}

bool CImageOverlayEffect::IsParallelCommand(const ImageOverlayCommand::IOC commandType) const {
	switch (commandType) {
		case ImageOverlayCommand::ParallelFadeToAlpha:
		case ImageOverlayCommand::ParallelGrow:
		case ImageOverlayCommand::ParallelJitter:
		case ImageOverlayCommand::ParallelMove:
		case ImageOverlayCommand::ParallelMoveTo:
		case ImageOverlayCommand::ParallelRotate:
			return true;
		default:
			return false;
	}
}