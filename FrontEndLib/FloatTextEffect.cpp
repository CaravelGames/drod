// $Id$

/****** BEGIN LICENSE BLOCK *****
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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "FloatTextEffect.h"
#include "BitmapManager.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/UtilFuncs.h>

//
//Public methods.
//

//*****************************************************************************
CFloatTextEffect::CFloatTextEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,           //(in) Required params for CEffect 
	const UINT wX, const UINT wY,  //(in) Local location of text (pixels)
	const WSTRING& text,           //(in) Text to display.
	const SURFACECOLOR& color,     //(in) Color of text
	const UINT eSetFontType,       //(in) [F_Small]
	const Uint32 dwDuration,       //(in) Time to display [1000ms]
	const float speed,             //(in) Movement speed (ms/pixel) [12ms]
	const bool bFitToParent,       //(in) if set, then ensure widget displays
	                               //     initially within parent area [default=true]
	const bool bPerPixelTransparency)
	: CEffect(pSetWidget, dwDuration, EFFECTLIB::EFLOATTEXT)
	, fX(float(wX)), fY(float(wY))
	, text(text)
	, color(color)
	, eFontType(eSetFontType)
	, speed(speed)
	, pTextSurface(NULL)
	, bPerPixelTransparency(bPerPixelTransparency)
{
	//Translate local pixel location to screen location.
	ASSERT(this->pOwnerWidget);
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->fX += OwnerRect.x;
	this->fY += OwnerRect.y;

	PrepWidget();

	if (bFitToParent)
	{
		//Fit to parent width.
		const int overshootX = (int(this->fX) + int(this->w)) - (OwnerRect.x + int(OwnerRect.w));
		if (overshootX > 0)
		{
			this->fX -= overshootX;
			if (this->fX < OwnerRect.x)
				this->fX = float(OwnerRect.x);
		}
	}
}

//*****************************************************************************
CFloatTextEffect::CFloatTextEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,           //(in) Required params for CEffect 
	const UINT wX, const UINT wY,  //(in) Local location of text (pixels)
	const int num,                 //(in) Number to display.
	const SURFACECOLOR& color,     //(in) Color of text
	const UINT eSetFontType,       //(in) [F_Small]
	const Uint32 dwDuration,       //(in) Time to display [1000ms]
	const float speed,             //(in) Movement speed (ms/pixel) [12ms]
	const bool bFitToParent,       //(in) if set, then ensure widget displays
	                               //     initially within parent area [default=true]
	const bool bPerPixelTransparency)
	: CEffect(pSetWidget, dwDuration, EFFECTLIB::EFLOATTEXT)
	, fX(float(wX)), fY(float(wY))
	, color(color)
	, eFontType(eSetFontType)
	, speed(speed)
	, pTextSurface(NULL)
	, bPerPixelTransparency(bPerPixelTransparency)
{
	WCHAR temp[12];
	this->text = _itoW(num, temp, 10);

	//Translate local pixel location to screen location.
	ASSERT(this->pOwnerWidget);
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->fX += OwnerRect.x;
	this->fY += OwnerRect.y;

	PrepWidget();

	if (bFitToParent)
	{
		//Fit to parent width.
		const int overshootX = (int(this->fX) + int(this->w)) - (OwnerRect.x + int(OwnerRect.w));
		if (overshootX > 0)
		{
			this->fX -= overshootX;
			if (this->fX < OwnerRect.x)
				this->fX = float(OwnerRect.x);
		}
	}
}

//*****************************************************************************
CFloatTextEffect::~CFloatTextEffect()
{
	SDL_FreeSurface(this->pTextSurface);
}

//*****************************************************************************
void CFloatTextEffect::PrepWidget()
//Set widget dimensions and prepare surface to render.
{
	g_pTheFM->GetTextWidthHeight(this->eFontType, this->text.c_str(), 
			this->w, this->h);

	//Area of effect.
	this->dirtyRects.clear();
	SDL_Rect rect;
	this->dirtyRects.push_back(rect);

	static const UINT outlineWidth = 1;

	//Render text to internal surface to avoid re-rendering each frame.
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);

	const UINT width = this->w + outlineWidth*2 + (this->bPerPixelTransparency ? 4 : 0);
	const UINT height = this->h + outlineWidth*2 + (this->bPerPixelTransparency ? 2 : 0);
	const SDL_Color origColor = g_pTheFM->GetFontColor(this->eFontType);
	const bool bOrigAntiAlias = g_pTheFM->GetFontAntiAlias(this->eFontType);
	const UINT wOrigOutlineWidth = g_pTheFM->GetFontOutline(this->eFontType);
	SDL_Color sdlColor = {this->color.byt1, this->color.byt2, this->color.byt3, 0};

	g_pTheFM->SetFontAntiAlias(this->eFontType, this->bPerPixelTransparency);
	g_pTheFM->SetFontOutline(this->eFontType, outlineWidth);
	g_pTheFM->SetFontColor(this->eFontType, sdlColor);

	if (this->bPerPixelTransparency) {
		this->pTextSurface = CBitmapManager::ConvertSurface(
			SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
		const Uint32 bg_color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_FillRect(this->pTextSurface, NULL, bg_color);

		//Draw text (outlined, w/ anti-aliasing).
		g_pTheFM->DrawTextToRect(this->eFontType, this->text.c_str(),
				0, 0, width, height, this->pTextSurface);
	} else {
		this->pTextSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, width, height,
				g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		ASSERT(this->pTextSurface);

		//Make text background transparent.
		SDL_Color BG = g_pTheFM->GetFontBackColor(this->eFontType);
		const Uint32 TransparentColor = SDL_MapRGB(this->pTextSurface->format,
			BG.r, BG.g, BG.b);
		SDL_FillRect(this->pTextSurface, NULL, TransparentColor);
		SetColorKey(this->pTextSurface, SDL_TRUE, TransparentColor);

		//Draw text (outlined, w/o anti-aliasing).
		g_pTheFM->DrawTextXY(this->eFontType, this->text.c_str(), this->pTextSurface,
				outlineWidth, outlineWidth);
	}

	g_pTheFM->SetFontColor(this->eFontType, origColor);
	g_pTheFM->SetFontAntiAlias(this->eFontType, bOrigAntiAlias);
	g_pTheFM->SetFontOutline(this->eFontType, wOrigOutlineWidth);

	SDL_Rect TextRect = MAKE_SDL_RECT(0, 0, this->pTextSurface->w, this->pTextSurface->h);
	this->srcRect = TextRect;
}

//*****************************************************************************
bool CFloatTextEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	const float fMultiplier = wDeltaTime / this->speed;   //1 pixel / #ms
	this->fY -= fMultiplier;   //float upward

	//Clip to owner widget.
	this->drawRect = MAKE_SDL_RECT(this->fX, this->fY,
		this->srcRect.w, this->srcRect.h);
	SDL_Rect SrcRect = this->srcRect;
	this->pOwnerWidget->GetRect(this->clipRect);

	const Uint32 fadeDuration = this->dwDuration / 4;
	const Uint32 periodBeforeFade = this->dwDuration - fadeDuration;
	Uint32 dwOpacity = 255;
	if (dwTimeElapsed > periodBeforeFade)
		dwOpacity = static_cast<Uint32>(dwOpacity * (1.0 - (dwTimeElapsed - periodBeforeFade) / float(fadeDuration)));
	this->nOpacity = min(255, dwOpacity);

	if (this->bPerPixelTransparency) {
		//manual clipping potentially required
		if (this->drawRect.y < this->clipRect.y) {
			const int delta = this->clipRect.y - this->drawRect.y;
			if (delta >= SrcRect.h)
				return false; //permanently out of display area
			
			this->drawRect.y = this->clipRect.y;
			SrcRect.y += delta;
			SrcRect.h -= delta;
			this->drawRect.h -= delta;
		}
		if (this->drawRect.y < 0) {
			ASSERT(!"Invalid display region");
			this->drawRect.y = 0; //routine doesn't support drawing outside rect
		}
	}

	//Dirty screen area.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0] = this->drawRect;

	return true;
}

//*****************************************************************************
void CFloatTextEffect::Draw(SDL_Surface& pDestSurface)
{
	//Clip to owner widget.
	this->drawRect = MAKE_SDL_RECT(this->fX, this->fY,
			this->srcRect.w, this->srcRect.h);
	SDL_Rect SrcRect = this->srcRect;

	const Uint32 fadeDuration = this->dwDuration / 4;
	const Uint32 periodBeforeFade = this->dwDuration - fadeDuration;
	Uint32 dwOpacity = 255;
	if (dwTimeElapsed > periodBeforeFade)
		dwOpacity = static_cast<Uint32>(dwOpacity * (1.0 - (dwTimeElapsed - periodBeforeFade) / float(fadeDuration)));
	const Uint8 nOpacity = dwOpacity >= 255 ? 255 : dwOpacity;

	//Blit.
	SDL_SetClipRect(&pDestSurface, &this->clipRect);
	if (this->bPerPixelTransparency) {
		g_pTheBM->BlitAlphaSurfaceWithTransparency(SrcRect, this->pTextSurface, this->drawRect, &pDestSurface, this->nOpacity);
	} else {
		g_pTheBM->BlitSurface(this->pTextSurface, &SrcRect, &pDestSurface, &this->drawRect, this->nOpacity);
	}
	SDL_SetClipRect(&pDestSurface, NULL);

	//Dirty screen area.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0] = this->drawRect;
}
