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
		: startMS(0), endMS(0)
		, startAlpha(0)
		, startX(0), startY(0)
		, startAngle(0), startScale(0)
	{ }

	Uint32 startMS, endMS;
	Uint8 startAlpha;
	int startX, startY;
	int startAngle;
	int startScale;
	UINT endTurn;
};

struct ConcurrentCommand
{
	ConcurrentCommand(const ImageOverlayCommand& command, const CommandExecution& ce)
		: command(command)
		, ce(ce)
	{ }

	ImageOverlayCommand command;
	CommandExecution ce;
};

class CImageOverlay;
class CRoomWidget;
class CImageOverlayEffect : public CEffect
{
public:
	CImageOverlayEffect(CWidget *pSetWidget, const CImageOverlay *pImageOverlay, const UINT turnNo,
			const Uint32 dwStartTime);
	virtual ~CImageOverlayEffect();

	virtual bool Draw(SDL_Surface* pDestSurface);
	virtual long GetDrawSequence() const { return drawSequence; }

	UINT getInstanceID() const { return instanceID; }
	UINT getStartTurn() const { return turnNo; }

private:
	bool AdvanceState();

	bool BeginCommand(const ImageOverlayCommand& command, bool& bProcessNextCommand);
	void ContinueCommand(const ImageOverlayCommand& command, const CommandExecution& ce, const Uint32 dwNow);
	void FinishCommand(const ImageOverlayCommand& command, const CommandExecution& ce);
	Uint32 ProcessConcurrentCommands(const Uint32 dwNow);
	bool RestartCommands();

	void DisplayImage(SDL_Surface* pDestSurface);

	void InitParams();

	bool PositionDisplayInsideRect(SDL_Rect& src, SDL_Rect& dest,
			int display_x, int display_y) const;

	void PrepareAlteredImage();

	long drawSequence;

	SDL_Surface *pImageSurface, *pAlteredSurface;
	bool bPrepareAlteredImage;

	int x, y;
	Uint8 alpha;
	int angle;
	int scale;
	int jitter;
	SDL_Rect sourceClipRect;

	ImageOverlayCommands commands;
	UINT index;
	int loopIteration, maxLoops;
	Uint32 startOfNextEffect;

	CommandExecution ce;
	vector<ConcurrentCommand> concurrentCommands;

	CRoomWidget *pRoomWidget;
	UINT turnNo;

	UINT instanceID; //for merging effect sets during play after move undo/checkpoint restore
};

#endif //...#ifndef IMAGEOVERLAYEFFECT_H
