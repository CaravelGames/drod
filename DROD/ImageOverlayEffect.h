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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2012
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef IMAGEOVERLAYEFFECT_H
#define IMAGEOVERLAYEFFECT_H

#include "../DRODLib/CharacterCommand.h"
#include <FrontEndLib/Effect.h>

struct CommandExecution
{
	CommandExecution()
		: duration(0), remainingTime(0)
		, startAlpha(0)
		, startX(0), startY(0)
		, startAngle(0), startScale(0)
	{ }

	Uint32 duration, remainingTime;
	Uint8 startAlpha;
	int startX, startY;
	int startAngle;
	int startScale;
	UINT endTurn;
};

struct ParallelCommand
{
	ParallelCommand(const ImageOverlayCommand& command, const CommandExecution& ce)
		: command(command)
		, executionState(ce)
	{ }

	ImageOverlayCommand command;
	CommandExecution executionState;
};

class CImageOverlay;
class CRoomWidget;
class CImageOverlayEffect : public CEffect
{
public:
	CImageOverlayEffect(CWidget *pSetWidget, const CImageOverlay *pImageOverlay, const UINT turnNo,
			const Uint32 dwStartTime);
	virtual ~CImageOverlayEffect();

	virtual long GetDrawSequence() const { return drawSequence; }

	UINT getInstanceID() const { return instanceID; }
	UINT getStartTurn() const { return turnNo; }

	int getGroup() const;

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	bool AdvanceState(const UINT wDeltaTime);

	Uint32 UpdateCommand(const ImageOverlayCommand& command, CommandExecution& ce, const Uint32 dwRemainingTime);
	Uint32 UpdateParallelCommands(const Uint32 dwDeltaTime);
	bool IsCurrentCommandFinished() const;
	bool CanContinuePlayingEffect(const Uint32 dwRemainingTime) const;
	bool CanLoop() const;
	inline bool IsCommandQueueFinished() const;
	bool IsImageDrawn();
	void PrepareDrawProperties();
	void StartNextCommand();
	void FinishCommand(const ImageOverlayCommand& command, const CommandExecution& ce);

	bool IsTimeBasedCommand(const ImageOverlayCommand::IOC commandType) const;
	bool IsTurnBasedCommand(const ImageOverlayCommand::IOC commandType) const;
	bool IsInstantCommand(const ImageOverlayCommand::IOC commandType) const;
	bool IsParallelCommand(const ImageOverlayCommand::IOC commandType) const;

	void InitParams();

	bool PositionDisplayInsideRect(SDL_Rect& src, SDL_Rect& dest,
			int display_x, int display_y) const;

	void PrepareAlteredImage();

	long drawSequence;

	SDL_Surface *pImageSurface, *pAlteredSurface;
	bool bPrepareAlteredImage;

	int x, y; // Real position of the image
	Uint8 alpha;
	int angle;
	int scale;
	int jitter;
	SDL_Rect sourceClipRect;

	// Properties used for the drawing
	int drawX, drawY; // Position to draw image after applying any active effects
	Uint8 drawAlpha;
	SDL_Rect drawSourceRect;
	SDL_Rect drawDestinationRect;

	ImageOverlayCommands commands;
	UINT index;
	int loopIteration, maxLoops;
	Uint32 startOfNextEffect;

	CommandExecution executionState;
	vector<ParallelCommand> parallelCommands;

	CRoomWidget *pRoomWidget;
	UINT turnNo;

	UINT instanceID; //for merging effect sets during play after move undo/checkpoint restore
};

#endif //...#ifndef IMAGEOVERLAYEFFECT_H
