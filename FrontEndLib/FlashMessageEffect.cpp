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

#include "FlashMessageEffect.h"
#include "FontManager.h"
#include "BitmapManager.h"
#include "Widget.h"

#include <BackEndLib/Assert.h>

#include <math.h>

static const float max_scale_factor = 0.1f;

//********************************************************************************
CFlashMessageEffect::CFlashMessageEffect(
//Constructor.
//
//
//Params:
	CWidget *pSetWidget,    //(in)  Should be a room widget or screen.
	const WCHAR *text,      //(in)  Text to display
	const int yOffset,		//(in)  Offset from center of parent, in pixels [default=0]
	const Uint32 wDuration, //(in)  How long to display (in milliseconds) [default=3000]
	const Uint32 fadeTime)  //fade out time at end of duration [default=1000]
	: CEffect(pSetWidget, wDuration, EFFECTLIB::EGENERIC)
	, pTextSurface(NULL)
	, yOffset(yOffset)
	, fadeTime(fadeTime)
	, movement(true)
	, bSlowExpansion(false)
	, bBlendedFontRender(false)
	, bCustomColor(false)
{
	ASSERT(pSetWidget->GetType() == WT_Room || pSetWidget->GetType() == WT_Screen);
	ASSERT(text);

	pSetWidget->GetRect(this->screenRect);
	this->dirtyRects.push_back(this->screenRect);

	this->wstrText = text;

	RenderText();
}

//********************************************************************************
CFlashMessageEffect::~CFlashMessageEffect()
{
	FreeTextSurface();
}

//********************************************************************************
bool CFlashMessageEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	UINT scaled_w = this->base_size.w;
	UINT scaled_h = this->base_size.h;

	ScaleText(dwTimeElapsed, scaled_w, scaled_h);

	//Center text in widget.
	const UINT xDraw = (this->screenRect.w - scaled_w) / 2;
	const UINT yDraw = this->yOffset + (this->screenRect.h - scaled_h) / 2;

	//Specify area of effect.
	ASSERT(this->dirtyRects.size() == 1);
	this->drawRect = MAKE_SDL_RECT(this->screenRect.x + xDraw, this->screenRect.y + yDraw,
		scaled_w, scaled_h);
	this->dirtyRects[0] = this->drawRect;

	UpdateOpacity(dwTimeElapsed);

	return true;
}

void CFlashMessageEffect::ScaleText(const Uint32& dwTimeElapsed, UINT& scaled_w, UINT& scaled_h)
{
	if (this->movement)
	{
		float size_delta;
		if (this->bSlowExpansion) {
			size_delta = (1.0f - max_scale_factor) + (2.0f * max_scale_factor * dwTimeElapsed / float(this->dwDuration));
		}
		else {
			//Pulse text size.
			static const float cycle = 1000.0f; //ms
			size_delta = 1.0f + (max_scale_factor * float(sin(TWOPI * dwTimeElapsed / cycle)));
		}
		scaled_w = UINT(size_delta * scaled_w);
		scaled_h = UINT(size_delta * scaled_h);
	}
}

void CFlashMessageEffect::UpdateOpacity(const Uint32& dwTimeElapsed)
{
	this->nOpacity = 255;
	if (g_pTheBM->bAlpha) {
		static const Uint32 FADE_IN_DURATION = 500; //ms
		if (dwTimeElapsed < FADE_IN_DURATION) {
			this->nOpacity = (Uint8)(255 * dwTimeElapsed / FADE_IN_DURATION);
		}
		else {
			const Uint32 time_left = this->dwDuration - dwTimeElapsed;
			if (time_left < this->fadeTime) {
				static const Uint8 start_opacity = 255;
				const float fFadePerMS = start_opacity / float(this->fadeTime);
				this->nOpacity = (Uint8)(time_left * fFadePerMS);
			}
		}
	}
}

//********************************************************************************
void CFlashMessageEffect::Draw(SDL_Surface& destSurface)
//Draws a pulsing message in the middle of the parent widget.
{
	SDL_Surface *pSurfaceToDraw = this->pTextSurface;
	SDL_Surface *pScaledSurface = NULL;
	if (this->movement)
	{
		//Scale.
		Uint8 *pSrcPixel = (Uint8*)this->pTextSurface->pixels;
		pScaledSurface = g_pTheBM->ScaleSurface(this->pTextSurface, pSrcPixel,
				this->base_size.w, this->base_size.h,
				this->drawRect.w, this->drawRect.h);
		if (!pScaledSurface)
			return;
		pSurfaceToDraw = pScaledSurface;
	}

	if (this->nOpacity < 255)
		if (this->bBlendedFontRender)
			g_pTheBM->SetSurfaceAlpha(pSurfaceToDraw, this->nOpacity);
		else
			EnableSurfaceBlending(pSurfaceToDraw, this->nOpacity);

	SDL_BlitSurface(pSurfaceToDraw, NULL, &destSurface, &this->drawRect);

	if (pScaledSurface)
		SDL_FreeSurface(pScaledSurface);
}

//********************************************************************************
void CFlashMessageEffect::RenderText()
//Creates a surface with the text to display, so it doesn't have to be rerendered each frame.
{
	ASSERT(!this->pTextSurface);

	const float scale_factor = this->movement ? max_scale_factor : 0.0f;

	const UINT max_w = UINT(this->screenRect.w * (1.0f - scale_factor));
	const UINT max_h = UINT(this->screenRect.h * (1.0f - scale_factor));

	static const UINT eDrawFont = FONTLIB::F_FlashMessage;
	UINT w, h;
	g_pTheFM->GetTextWidthHeight(eDrawFont, this->wstrText.c_str(), w, h);
	if (w > max_w)
		w = max_w;
	if (h > max_h)
		h = max_h;
	this->dirtyRects[0].w = w;
	this->dirtyRects[0].h = h;
	this->base_size = this->dirtyRects[0];

	ASSERT(this->dirtyRects.size() == 1);

	SDL_Color tempColor;
	if (this->bCustomColor) {
		tempColor = g_pTheFM->GetFontColor(eDrawFont);
		g_pTheFM->SetFontColor(eDrawFont, this->customColor);
	}

	this->bBlendedFontRender = g_pTheFM->GetFontAntiAlias(eDrawFont) && g_pTheFM->GetFontOutline(eDrawFont);
	int y_font_offset = 0;
	Uint32 TransparentColor;
	if (this->bBlendedFontRender) {
		y_font_offset = -6;

		this->pTextSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, w, h, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
		const Uint32 color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_FillRect(this->pTextSurface, NULL, color);
	} else {
		this->pTextSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, w, h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		ASSERT(this->pTextSurface);

		//make entire bg transparent
		const SDL_Color BG = g_pTheFM->GetFontBackColor(eDrawFont);
		TransparentColor = SDL_MapRGB(this->pTextSurface->format,
			BG.r, BG.g, BG.b);
		SDL_FillRect(this->pTextSurface, NULL, TransparentColor);
	}

	g_pTheFM->DrawTextXY(eDrawFont, this->wstrText.c_str(), this->pTextSurface, 0, y_font_offset,
		max_w, max_h);

	if (!this->bBlendedFontRender)
		SetColorKey(this->pTextSurface, SDL_TRUE, TransparentColor);

	if (this->bCustomColor)
		g_pTheFM->SetFontColor(eDrawFont, tempColor);
}

//********************************************************************************
void CFlashMessageEffect::SetColor(int r, int g, int b)
{
	FreeTextSurface();

	this->bCustomColor = true;
	this->customColor.r = (unsigned char)r;
	this->customColor.g = (unsigned char)g;
	this->customColor.b = (unsigned char)b;

	RenderText();
}

//********************************************************************************
void CFlashMessageEffect::FreeTextSurface()
{
	if (this->pTextSurface) {
		SDL_FreeSurface(this->pTextSurface);
		this->pTextSurface = NULL;
	}
}