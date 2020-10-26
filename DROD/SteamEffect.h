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

#ifndef STEAMEFFECT_H
#define STEAMEFFECT_H

#include "DrodEffect.h"
#include <BackEndLib/Coord.h>

//*****************************************************************************
class CSteamEffect : public CEffect
{
public:
	CSteamEffect(CWidget *pSetWidget, const CCoord &origin);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

	void UpdateFrame();

private:
	SDL_Rect screenRect;

	UINT nInitialDrawY;
	UINT wDrawX, wDrawY;
	UINT wDrawClipY;     //Amount of pixesl from the top of the sprite to not draw because they're outside room bounds
	UINT wDrawTile;
	Uint8 nDrawOpacity;
};

#endif   //...#ifndef STEAMEFFECT_H
