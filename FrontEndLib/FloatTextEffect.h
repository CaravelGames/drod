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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef FLOATTEXTEFFECT_H
#define FLOATTEXTEFFECT_H

#include "Effect.h"
#include "FontManager.h"
#include <BackEndLib/Coord.h>

//*****************************************************************************
class CFloatTextEffect : public CEffect
{
public:
	CFloatTextEffect(CWidget *pSetWidget, const UINT wX, const UINT wY, const WSTRING& text,
			const SURFACECOLOR& color,
			const UINT eSetFontType=FONTLIB::F_Small,
			const Uint32 dwDuration=1000,
			const float speed=12.0,
			const bool bFitToParent=true,
			const bool bPerPixelTransparency=true);
	CFloatTextEffect(CWidget *pSetWidget, const UINT wX, const UINT wY, const int num,
			const SURFACECOLOR& color,
			const UINT eSetFontType=FONTLIB::F_Small,
			const Uint32 dwDuration=1000,
			const float speed=12.0,
			const bool bFitToParent=true,
			const bool bPerPixelTransparency=true);
	virtual ~CFloatTextEffect();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	void           PrepWidget();

	float fX, fY; //real position
	WSTRING text; //text to display
	UINT    w, h; //rendered text dimensions

	SURFACECOLOR color;
	UINT         eFontType;
	float        speed;

	SDL_Rect srcRect;
	SDL_Surface *  pTextSurface;
	bool bPerPixelTransparency;

	Uint8 nOpacity;
	SDL_Rect clipRect;
	SDL_Rect drawRect;
};

#endif   //...#ifndef FLOATTEXTEFFECT_H
