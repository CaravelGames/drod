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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TextEffect.h"
#include "FontManager.h"
#include "BitmapManager.h"
#include "Widget.h"

#include <BackEndLib/Assert.h>

//********************************************************************************
CTextEffect::CTextEffect(
//Constructor.
//
//
//Params:
	CWidget *pSetWidget,    //(in)  Should be a room widget.
	const WCHAR *text,      //(in)  Text to display
	const UINT eFont,       //(in)  Font to use
	const Uint32 dwFadeIn,	//(in)	How long to fade text in (in milliseconds) [default = 0ms]
	const Uint32 dwDuration, //(in)  How long to display (in milliseconds) [default = 0, means display indefinitely]
	const bool bFadeOut,     //(in)  If true, specifies to fade out instead of fade in [default=false]
	const bool bPerPixelTransparency)
	: CEffect(pSetWidget, dwDuration, EFFECTLIB::ETEXT)
	, pTextSurface(NULL)
	, dwDuration(dwDuration)
	, dwFadeIn(dwFadeIn)
	, bFadeOut(bFadeOut)
	, bPerPixelTransparency(bPerPixelTransparency)
	, nOpacity(255)
{
	pSetWidget->GetRect(this->screenRect);
	this->dirtyRects.push_back(this->screenRect);

	SetText(text, eFont);

	//Center text in widget by default.
	Move((this->screenRect.w - this->dirtyRects[0].w) / 2,
			(this->screenRect.h - this->dirtyRects[0].h) / 2);

	this->drawRect = MAKE_SDL_RECT(0, 0, 0, 0);
}

//********************************************************************************
CTextEffect::~CTextEffect()
{
	SDL_FreeSurface(this->pTextSurface);
}

//********************************************************************************
bool CTextEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
//Draws text in the middle of the parent widget.
{
	//Specify area of effect.
	ASSERT(this->dirtyRects.size() == 1);
	this->drawRect = MAKE_SDL_RECT(this->screenRect.x + this->nX, this->screenRect.y + this->nY,
		this->dirtyRects[0].w, this->dirtyRects[0].h);
	this->dirtyRects[0] = this->drawRect;

	//Fade text in.
	const Uint32 dwOpacity = static_cast<Uint32>(255.0 *
		(this->bFadeOut ?
			(dwTimeElapsed < this->dwFadeIn ? 1.0 : 1.0 - (dwTimeElapsed - this->dwFadeIn) / float(this->dwDuration - this->dwFadeIn)) :
			this->dwFadeIn ? dwTimeElapsed / (float)this->dwFadeIn : 1.0));
	this->nOpacity = dwOpacity >= 255 ? 255 : dwOpacity;

	return true;
}

//********************************************************************************
void CTextEffect::Draw(SDL_Surface& destSurface)
//Draws text in the middle of the parent widget.
{
	if (this->nOpacity > 0) {
		if (this->bPerPixelTransparency) {
			SDL_Rect src = MAKE_SDL_RECT(0, 0, this->pTextSurface->w, this->pTextSurface->h);
			g_pTheBM->BlitAlphaSurfaceWithTransparency(src, this->pTextSurface, this->drawRect, &destSurface, this->nOpacity);
		} else {
			if (g_pTheBM->bAlpha) {
				EnableSurfaceBlending(this->pTextSurface, nOpacity);
			} else {
				DisableSurfaceBlending(this->pTextSurface); //Remove transparency.
			}
			SDL_BlitSurface(this->pTextSurface, NULL, &destSurface, &this->drawRect);
		}
	}
}

//********************************************************************************
void CTextEffect::Move(const int nX, const int nY)
{
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->nX = nX;
	this->dirtyRects[0].y = this->nY = nY;
}

//********************************************************************************
void CTextEffect::SetText(const WCHAR *text, const UINT eFont)
{
	ASSERT(text);
	this->wstrText = text;

	//Draw text to surface so it doesn't have to be rendered each frame.
	UINT w, h;
	g_pTheFM->GetTextRectHeight(eFont, this->wstrText.c_str(), this->screenRect.w, w, h);
	if (!w) w = 1; //to avoid SDL 1.2 crashes with empty text
	if (!h) h = 1;
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].w = w;
	this->dirtyRects[0].h = h;

	if (this->pTextSurface) {
		SDL_FreeSurface(this->pTextSurface);
		this->pTextSurface = NULL;
	}

	if (this->bPerPixelTransparency) {
		this->pTextSurface = CBitmapManager::ConvertSurface(
			SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
		const Uint32 color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_FillRect(this->pTextSurface, NULL, color);

		g_pTheFM->DrawTextToRect(eFont, this->wstrText.c_str(),
				0, 0, w, h, this->pTextSurface);
	} else {
		//Surface color key transparency.
		this->pTextSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, w, h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		ASSERT(this->pTextSurface);

		SDL_Color BG = g_pTheFM->GetFontBackColor(eFont);
		const Uint32 TransparentColor = SDL_MapRGB(this->pTextSurface->format,
			BG.r, BG.g, BG.b);
		SDL_FillRect(this->pTextSurface, NULL, TransparentColor);   //make entire bg transparent
		g_pTheFM->DrawTextToRect(eFont, this->wstrText.c_str(), 0, 0, w, h, this->pTextSurface);
		SetColorKey(this->pTextSurface, SDL_TRUE, TransparentColor);
	}
}
