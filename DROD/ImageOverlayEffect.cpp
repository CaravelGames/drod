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

#include "ImageOverlayEffect.h"
#include "DrodBitmapManager.h"
#include "DrodEffect.h"
#include "RoomWidget.h"
#include "../DRODLib/CharacterCommand.h"
#include "../DRODLib/CurrentGame.h"

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
	: CEffect(pSetWidget, EIMAGEOVERLAY)
	, drawSequence(++nextDrawSequence)
	, pImageSurface(NULL), pAlteredSurface(NULL)
	, bPrepareAlteredImage(false)
	, x(0), y(0)
	, alpha(255)
	, angle(0), scale(ORIGINAL_SCALE)
	, jitter(0)
	, index(UINT(-1))
	, loopIteration(0), maxLoops(0)
	, startOfNextEffect(dwStartTime)
	, pRoomWidget(NULL)
	, turnNo(turnNo)
	, instanceID(0)
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
	this->angle = 0;
	this->jitter = 0;
	this->scale = ORIGINAL_SCALE;

	static const SDL_Rect rect = {0,0,0,0};
	this->sourceClipRect = rect;
	this->concurrentCommands.clear();
}

bool CImageOverlayEffect::Draw(SDL_Surface* pDestSurface)
{
	if (!this->pImageSurface)
		return false;

	if (!AdvanceState())
		return false;

	if (this->bPrepareAlteredImage) {
		PrepareAlteredImage();
		this->bPrepareAlteredImage = false;
	}

	DisplayImage(pDestSurface);
	return true;
}

bool CImageOverlayEffect::AdvanceState()
{
	const Uint32 dwNow = SDL_GetTicks();

	const Uint32 maxCompletedEndMS = ProcessConcurrentCommands(dwNow);

	const CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	const UINT gameTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;

	const bool bExecuteNextCommand =
		this->index >= this->commands.size() || //either at start, or waiting for parallel commands to complete
		dwNow >= this->ce.endMS ||
		gameTurn >= this->ce.endTurn;
	if (bExecuteNextCommand)
	{
		if (this->index < this->commands.size()) {
			const ImageOverlayCommand& command = this->commands[this->index];
			FinishCommand(command, this->ce);
		}

		//Advance to next display command.
		bool bLoopedThisTurn = false;
		bool bProcessNextCommand;
		do {
			if (this->index != this->commands.size())
				++this->index;

			if (this->index >= this->commands.size())
			{
				//No more commands to execute.

				//wait until any parallel commands have finished displaying
				if (!this->concurrentCommands.empty())
					return true;

				if (bLoopedThisTurn)
					return false; //looping without any display -- stop processing

				//When command advancement was only waiting on concurrent effects to complete,
				//set time for starting the next loop to the end of the final concurrent command
				if (maxCompletedEndMS > this->startOfNextEffect)
					this->startOfNextEffect = maxCompletedEndMS;

				if (!RestartCommands())
					return false;
				bLoopedThisTurn = true;
			}

			bProcessNextCommand = false;

			const ImageOverlayCommand& command = this->commands[this->index];
			if (!BeginCommand(command, bProcessNextCommand))
				return false;
		} while (bProcessNextCommand);

		this->startOfNextEffect = this->ce.endMS;
	}

	ASSERT(this->index < this->commands.size());
	ContinueCommand(this->commands[this->index], this->ce, dwNow);

	return true;
}

bool CImageOverlayEffect::BeginCommand(const ImageOverlayCommand& command, bool& bProcessNextCommand)
{
	this->ce.startMS = this->startOfNextEffect;
	this->ce.endMS = UINT(-1);
	this->ce.endTurn = UINT(-1);

	const ImageOverlayCommand::IOC curCommand = command.type;
	int val = command.val[0];
	switch (curCommand) {
		case ImageOverlayCommand::CancelAll:
		case ImageOverlayCommand::CancelLayer:
			//these must be handled by some owning widget
			//stop processing when encountered
		return false;

		case ImageOverlayCommand::Center:
			this->x = (int(this->pRoomWidget->GetW()) - int(this->pImageSurface->w)) / 2;
			this->y = (int(this->pRoomWidget->GetH()) - int(this->pImageSurface->h)) / 2;
			bProcessNextCommand = true;
		break;

		case ImageOverlayCommand::DisplayDuration:
			this->ce.endMS = this->ce.startMS + max(0, val);
		break;
		case ImageOverlayCommand::DisplayRect:
			this->sourceClipRect.x = max(0, val);
			this->sourceClipRect.y = max(0, command.val[1]);
			this->sourceClipRect.w = min(max(0, command.val[2]), this->pImageSurface->w);
			this->sourceClipRect.h = min(max(0, command.val[3]), this->pImageSurface->h);
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::DisplaySize:
			this->sourceClipRect.w = min(max(0, val), this->pImageSurface->w);
			this->sourceClipRect.h = min(max(0, command.val[1]), this->pImageSurface->h);
			bProcessNextCommand = true;
		break;

		case ImageOverlayCommand::FadeToAlpha:
			this->ce.endMS = this->ce.startMS + max(0, command.val[1]);
			this->ce.startAlpha = this->alpha;
		break;
		case ImageOverlayCommand::ParallelFadeToAlpha:
		{
			ConcurrentCommand c(command, this->ce);
			c.ce.endMS = this->ce.startMS + max(0, command.val[1]);
			c.ce.startAlpha = this->alpha;
			this->concurrentCommands.push_back(c);

			bProcessNextCommand = true;
		}
		break;

		case ImageOverlayCommand::Grow:
			this->ce.startScale = this->scale;
			this->ce.endMS = this->ce.startMS + max(0, command.val[1]);
		break;
		case ImageOverlayCommand::ParallelGrow:
		{
			ConcurrentCommand c(command, this->ce);
			c.ce.startScale = this->scale;
			c.ce.endMS = this->ce.startMS + max(0, command.val[1]);
			this->concurrentCommands.push_back(c);

			bProcessNextCommand = true;
		}
		break;

		case ImageOverlayCommand::Jitter:
			this->jitter = max(0, val);
			this->ce.endMS = this->ce.startMS + max(0, command.val[1]);
		break;
		case ImageOverlayCommand::ParallelJitter:
		{
			this->jitter = max(0, val);

			ConcurrentCommand c(command, this->ce);
			c.ce.endMS = this->ce.startMS + max(0, command.val[1]);
			this->concurrentCommands.push_back(c);

			bProcessNextCommand = true;
		}
		break;

		case ImageOverlayCommand::Layer:
			bProcessNextCommand = true;
		break;

		case ImageOverlayCommand::Loop:
			this->maxLoops = val;
			bProcessNextCommand = true;
		break;

		case ImageOverlayCommand::Move:
		case ImageOverlayCommand::MoveTo:
			this->ce.startX = this->x;
			this->ce.startY = this->y;
			this->ce.endMS = this->ce.startMS + max(0, command.val[2]);
		break;
		case ImageOverlayCommand::ParallelMove:
		case ImageOverlayCommand::ParallelMoveTo:
		{
			ConcurrentCommand c(command, this->ce);
			c.ce.startX = this->x;
			c.ce.startY = this->y;
			c.ce.endMS = this->ce.startMS + max(0, command.val[2]);
			this->concurrentCommands.push_back(c);

			bProcessNextCommand = true;
		}
		break;

		case ImageOverlayCommand::Rotate:
			this->ce.startAngle = this->angle;
			this->ce.endMS = this->ce.startMS + max(0, command.val[1]);
		break;
		case ImageOverlayCommand::ParallelRotate:
		{
			ConcurrentCommand c(command, this->ce);
			c.ce.startAngle = this->angle;
			c.ce.endMS = this->ce.startMS + max(0, command.val[1]);
			this->concurrentCommands.push_back(c);

			bProcessNextCommand = true;
		}
		break;

		case ImageOverlayCommand::Scale:
			this->scale = clipScale(val);
			this->bPrepareAlteredImage = true;
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::SetAlpha:
			this->alpha = clipAlpha(val);
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::SetAngle:
			this->angle = clipAngle(val);
			this->bPrepareAlteredImage = true;
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::SetX:
			this->x = val;
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::SetY:
			this->y = val;
			bProcessNextCommand = true;
		break;
		case ImageOverlayCommand::SrcXY:
			this->sourceClipRect.x = max(0, val);
			this->sourceClipRect.y = max(0, command.val[1]);
			bProcessNextCommand = true;
		break;

		case ImageOverlayCommand::TurnDuration:
		{
			const CDbRoom *pRoom = this->pRoomWidget->GetRoom();
			const UINT gameTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;
			this->ce.endTurn = gameTurn + max(0, val);
		}
		break;

		default:
			return false;
	}
	return true;
}

void CImageOverlayEffect::ContinueCommand(const ImageOverlayCommand& command, const CommandExecution& ce, const Uint32 dwNow)
{
	const UINT durationMS = ce.endMS - ce.startMS;
	const float t = (dwNow - ce.startMS) / float(durationMS);
	switch (command.type) {
		case ImageOverlayCommand::FadeToAlpha:
		case ImageOverlayCommand::ParallelFadeToAlpha:
		{
			const int deltaAlpha = command.val[0] - ce.startAlpha;
			this->alpha = clipAlpha(ce.startAlpha + int(deltaAlpha * t));
		}
		break;
		case ImageOverlayCommand::Grow:
		case ImageOverlayCommand::ParallelGrow:
		{
			const int deltaScale = command.val[0];
			this->scale = clipScale(ce.startScale + int(deltaScale * t));
			this->bPrepareAlteredImage = true;
		}
		break;
		case ImageOverlayCommand::Jitter:
		case ImageOverlayCommand::ParallelJitter:
			//nothing to do here
		break;
		case ImageOverlayCommand::Move:
		case ImageOverlayCommand::ParallelMove:
		{
			const int deltaX = command.val[0];
			const int deltaY = command.val[1];
			this->x = ce.startX + int(deltaX * t);
			this->y = ce.startY + int(deltaY * t);
		}
		break;
		case ImageOverlayCommand::MoveTo:
		case ImageOverlayCommand::ParallelMoveTo:
		{
			const int deltaX = command.val[0] - ce.startX;
			const int deltaY = command.val[1] - ce.startY;
			this->x = ce.startX + int(deltaX * t);
			this->y = ce.startY + int(deltaY * t);
		}
		break;
		case ImageOverlayCommand::Rotate:
		case ImageOverlayCommand::ParallelRotate:
		{
			const int deltaAngle = command.val[0];
			this->angle = clipAngle(ce.startAngle + int(deltaAngle * t));
			this->bPrepareAlteredImage = true;
		}
		break;
		default: break;
	}
}

void CImageOverlayEffect::FinishCommand(const ImageOverlayCommand& command, const CommandExecution& ce)
{
	switch (command.type) {
		case ImageOverlayCommand::FadeToAlpha:
		case ImageOverlayCommand::ParallelFadeToAlpha:
			this->alpha = clipAlpha(command.val[0]);
			break;
		case ImageOverlayCommand::Grow:
		case ImageOverlayCommand::ParallelGrow:
			this->scale = clipScale(ce.startScale + command.val[0]);
			this->bPrepareAlteredImage = true;
			break;
		case ImageOverlayCommand::Jitter:
		case ImageOverlayCommand::ParallelJitter:
			this->jitter = 0;
			break;
		case ImageOverlayCommand::Move:
		case ImageOverlayCommand::ParallelMove:
			this->x = ce.startX + command.val[0];
			this->y = ce.startY + command.val[1];
			break;
		case ImageOverlayCommand::MoveTo:
		case ImageOverlayCommand::ParallelMoveTo:
			this->x = command.val[0];
			this->y = command.val[1];
			break;
		case ImageOverlayCommand::Rotate:
		case ImageOverlayCommand::ParallelRotate:
			this->angle = clipAngle(ce.startAngle + command.val[0]);
			this->bPrepareAlteredImage = true;
			break;
		case ImageOverlayCommand::TurnDuration:
			// This is required for any time-based command that runs afterwards to correctly
			// calculate the start time
			this->startOfNextEffect = SDL_GetTicks();
			break;
		default: break;
	}
}

Uint32 CImageOverlayEffect::ProcessConcurrentCommands(const Uint32 dwNow)
{
	if (this->concurrentCommands.empty())
		return 0;

	vector<ConcurrentCommand> continuingCommands;
	Uint32 maxCompletedEndMS = 0;

	for (vector<ConcurrentCommand>::const_iterator it=this->concurrentCommands.begin();
		it!=this->concurrentCommands.end(); ++it)
	{
		const ConcurrentCommand& c = *it;
		if (dwNow >= c.ce.endMS) {
			if (c.ce.endMS > maxCompletedEndMS)
				maxCompletedEndMS = c.ce.endMS;

			FinishCommand(c.command, c.ce);
		} else {
			ContinueCommand(c.command, c.ce, dwNow);
			continuingCommands.push_back(c);
		}
	}

	this->concurrentCommands = continuingCommands;

	return maxCompletedEndMS;
}

bool CImageOverlayEffect::RestartCommands()
{
	++this->loopIteration;
	if (this->maxLoops != ImageOverlayCommand::NO_LOOP_MAX && this->loopIteration >= this->maxLoops)
		return false; 

	//Loop
	InitParams();
	this->index = 0;
	return true;
}

void CImageOverlayEffect::DisplayImage(SDL_Surface* pDestSurface)
{
	SDL_Rect src = this->sourceClipRect;
	SDL_Rect dest;
	this->pOwnerWidget->GetRect(dest);

	SDL_Surface *pSrcSurface;
	int display_x = this->x, display_y = this->y;

	if (this->jitter) {
		display_x += ROUND(fRAND_MID(this->jitter));
		display_y += ROUND(fRAND_MID(this->jitter));
	}

	if (this->pAlteredSurface) {
		pSrcSurface = this->pAlteredSurface;
		if (!src.w)
			src.w = pSrcSurface->w;
		else if (src.w > pSrcSurface->w)
			src.w = pSrcSurface->w;
		if (!src.h)
			src.h = pSrcSurface->h;
		else if (src.h > pSrcSurface->h)
			src.h = pSrcSurface->h;
		display_x += (int(this->pImageSurface->w) - pSrcSurface->w) / 2; //keep centered
		display_y += (int(this->pImageSurface->h) - pSrcSurface->h) / 2; //keep centered
	} else {
		pSrcSurface = this->pImageSurface;
		if (!src.w)
			src.w = this->pImageSurface->w;
		if (!src.h)
			src.h = this->pImageSurface->h;
	}

	const Uint8 displayAlpha = Uint8(this->alpha * this->fOpacity);
	if (!displayAlpha || !PositionDisplayInsideRect(src, dest, display_x, display_y)) {
		this->dirtyRects[0].w = this->dirtyRects[0].h = 0;
	} else {
		if (!pDestSurface)
			pDestSurface = GetDestSurface();

		const bool bSurfaceAlpha = !this->pImageSurface->format->Amask && this->alpha < 255;
		if (bSurfaceAlpha) {
			EnableSurfaceBlending(pSrcSurface, displayAlpha);
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
			DisableSurfaceBlending(pSrcSurface);
		} else {
			g_pTheBM->BlitAlphaSurfaceWithTransparency(src, pSrcSurface, dest, pDestSurface, displayAlpha);
		}

		this->dirtyRects[0] = dest;
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
