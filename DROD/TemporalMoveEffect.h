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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TEMPORALMOVEEFFECT_H
#define TEMPORALMOVEEFFECT_H

#include "DrodEffect.h"
#include <FrontEndLib/AnimatedTileEffect.h>

//****************************************************************************************
class CRoomWidget;
class CTemporalMoveEffect : public CAnimatedTileEffect
{
public:
	CTemporalMoveEffect(CWidget *pSetWidget,
		const CMoveCoord &SetCoord, const UINT wTI, const bool isBump, const UINT type=ETEMPORALMOVE);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	UINT startX, startY;
	int deltaX, deltaY;
	bool isBump;

	Uint32 startDelay, endDelay;

	CRoomWidget *pRoomWidget;
	UINT wValidTurn;   //game turn this effect is valid for

	Uint8 nOpacity;
	UINT wDrawX;
	UINT wDrawY;
	SDL_Rect blitRect;
};

#endif //...#ifndef TEMPORALMOVEEFFECT_H
