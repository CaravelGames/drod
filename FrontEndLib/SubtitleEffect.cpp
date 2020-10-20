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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "SubtitleEffect.h"
#include "BitmapManager.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/UtilFuncs.h>

#include <limits>

//Background tool tip color.
const SURFACECOLOR Border = {0, 0, 0}; //black

const UINT wBorder = 2; //border buffer (pixels)
const UINT widthBuffer = (wBorder+1)*2; //how much width is unavailable for text rendering

const Uint32 dwWaitForNewText = 200; //ms -- additional time to show effect past duration until it beings to fade

//
//Public methods.
//

//*****************************************************************************
CSubtitleEffect::CSubtitleEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,                //(in)   Required params for CEffect 
	CMoveCoord *const pCoord,           //(in)   Desired location of text.
	const WCHAR *pwczSetText,           //(in)   Text that label will display.
	const SDL_Color& FGColor,           //(in)   FG color of text line
	const SURFACECOLOR& BGColor,        //(in)   BG color of text area
	const Uint32 dwDuration,            //(in)   Time till fade, 0 = indefinite [2000ms]
	const UINT wDisplayLines,           //(in)   Lines to display [default = 3]
	const UINT eSetFontType,            //(in)   [F_Small]
	const UINT maxWidth)
	: CEffect(pSetWidget, dwDuration, EFFECTLIB::ESUBTITLE)
	, bAttachedCoord(false)
	, pCoord(pCoord)
	, pSubtitles(NULL)
	, xOffset(CBitmapManager::CX_TILE), yOffset(CBitmapManager::CY_TILE) //position to lower-right of coord
	, eFontType(eSetFontType)
	, wDisplayLines(wDisplayLines)
	, maxWidth(maxWidth)
	, dwFadeDuration(500) //ms
	, pTextSurface(NULL)
	, BGColor(BGColor)
	, opacity(225)	//88%
{
	ASSERT(this->pOwnerWidget);

	this->texts.resize(wDisplayLines);
	if (pwczSetText)
		this->texts[0].text = pwczSetText;
	this->texts[0].color = FGColor;

	//Start effect at pCoord.
	this->x = this->y = (UINT)-1;
	this->w = this->h = 0;
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	this->dirtyRects.push_back(rect);
	SetLocation();

	PrepWidget();
}

//*****************************************************************************
CSubtitleEffect::~CSubtitleEffect()
{
	RemoveFromSubtitles();
	SDL_FreeSurface(this->pTextSurface);
	if (this->bAttachedCoord)
		delete this->pCoord;
}

//*****************************************************************************
void CSubtitleEffect::AddToSubtitles(SUBTITLES &subtitles)
{
	ASSERT(this->pCoord);
	ASSERT(!this->pSubtitles);
	this->pSubtitles = &subtitles;
	VERIFY(subtitles.insert(std::make_pair(this->pCoord,this)).second);
}

//*****************************************************************************
void CSubtitleEffect::FollowCoord(CMoveCoord *const pCoord, const bool bAttachedCoord) //[default=false]
//Provide the effect a (new) coord to track.
{
	ASSERT(pCoord);
	if (this->bAttachedCoord)
		delete this->pCoord;
	this->pCoord = pCoord;
	this->bAttachedCoord = bAttachedCoord;
}

//*****************************************************************************
long CSubtitleEffect::GetDrawSequence() const
{
	return std::numeric_limits<long>::max(); //draw last
}

//*****************************************************************************
void CSubtitleEffect::RemoveFromSubtitles()
{
	if (this->pSubtitles)
	{
		VERIFY(this->pSubtitles->erase(this->pCoord));
		this->pSubtitles = NULL;
	}
}

//*****************************************************************************
void CSubtitleEffect::PrepWidget()
//Set widget dimensions and prepare surface to render.
{
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);

	UINT wW, wH;
	this->w = OwnerRect.w;
	const UINT borderSize = wBorder*2 + 2;
	if (this->maxWidth > borderSize && this->w + borderSize > this->maxWidth)
		this->w = this->maxWidth - borderSize;
	GetTextWidthHeight(wW,wH);
	this->w = wW + borderSize;
	this->h = wH + borderSize;

	//Move away from edge of parent widget, if necessary.
	if (this->x + this->w >= static_cast<UINT>(OwnerRect.x + OwnerRect.w))
		this->x = OwnerRect.x + OwnerRect.w - this->w;
	if (this->y + this->h >= static_cast<UINT>(OwnerRect.y + OwnerRect.h))
		this->y = OwnerRect.y + OwnerRect.h - this->h;

	//Area of effect.
	this->dirtyRects.clear();
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	this->dirtyRects.push_back(rect);

	//Render text to internal surface to avoid re-rendering each frame.
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
	this->pTextSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	ASSERT(this->pTextSurface);
	//Ensure color is in same pixel color order as surface.
	const SURFACECOLOR BGColor = GetSurfaceColor(this->pTextSurface,
			this->BGColor.byt1, this->BGColor.byt2, this->BGColor.byt3);
	const SDL_Color TextBackColor = {this->BGColor.byt1, this->BGColor.byt2, this->BGColor.byt3, 0};

	//Fill rectangle and draw border.
	SDL_Rect BorderRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	SDL_Rect InnerRect = MAKE_SDL_RECT(1, 1, this->w-2, this->h-2);
	this->pOwnerWidget->DrawFilledRect(InnerRect, BGColor, this->pTextSurface);
	this->pOwnerWidget->DrawRect(BorderRect, Border, this->pTextSurface);

	//Draw text.
	UINT wLineH, wLineW;
	SDL_Rect TextRect = MAKE_SDL_RECT(wBorder, wBorder, this->w-widthBuffer, this->h-widthBuffer);
#ifdef RUSSIAN_BUILD
	static const int YOFFSET = 1;   //this font draws a tad too low
#else
	static const int YOFFSET = 4;   //this font always draws too low
#endif
	UINT wY = TextRect.y - YOFFSET;

	const SDL_Color origColor = g_pTheFM->GetFontColor(this->eFontType);
	const SDL_Color origTextBGColor = g_pTheFM->GetFontBackColor(this->eFontType);
	for (UINT wIndex=0; wIndex<this->wDisplayLines; ++wIndex)
	{
		if (this->texts[wIndex].text.size())
		{
			g_pTheFM->GetTextRectHeight(this->eFontType, this->texts[wIndex].text.c_str(), 
					TextRect.w, wLineW, wLineH);
			g_pTheFM->SetFontColor(this->eFontType, this->texts[wIndex].color);
			g_pTheFM->SetFontBackColor(this->eFontType, TextBackColor);
			g_pTheFM->DrawTextToRect(this->eFontType, this->texts[wIndex].text.c_str(), 
					TextRect.x+wBorder, wY, TextRect.w, wLineH, this->pTextSurface);
			wY += wLineH;
		}
	}
	g_pTheFM->SetFontColor(this->eFontType, origColor);
	g_pTheFM->SetFontBackColor(this->eFontType, origTextBGColor);

	if (g_pTheBM->bAlpha)
		SetAlpha(this->opacity);

	// Reset elapsed time otherwise the effect will end early
	this->dwTimeElapsed = 0;
}

//*****************************************************************************
void CSubtitleEffect::SetAlpha(const Uint8 opacity)
//Sets opacity of effect.
{
	this->opacity = opacity;
	if (!this->pTextSurface)
		return;

	AutodetectSurfaceBlending(this->pTextSurface, this->opacity);
}

//*****************************************************************************
void CSubtitleEffect::SetOffset(const int nX, const int nY)
//Change the default/current offset from 'pCoord'
{
	this->xOffset = nX;
	this->yOffset = nY;

	//Snap effect to this offset.
	this->x = this->y = (UINT)-1;
}

//*****************************************************************************
void CSubtitleEffect::GetTextWidthHeight(
//Gets width and height of text as it is drawn within label.
//
//Params:
	UINT &wW, UINT &wH)  //(out) Width and height of text.
const
{
	wW = wH = 0;
	if (this->texts[0].text.empty())
		return;

	//Ask font manager about it.
	UINT wLineH, wLineW;
	for (UINT wIndex=this->texts.size(); wIndex--; )
	{
		if (this->texts[wIndex].text.size())
		{
			g_pTheFM->GetTextRectHeight(this->eFontType, this->texts[wIndex].text.c_str(), 
					this->w-widthBuffer, wLineW, wLineH);
			if (wLineW > wW)
				wW = wLineW;
			wH += wLineH;
		}
	}
}

//*****************************************************************************
void CSubtitleEffect::AddTextLine(const WCHAR* pwczSetText, const Uint32 dwDuration,
	const SDL_Color& color, //[default=Black]
	const bool bSyncTextOffsetUpward) //[default=false]
//Add another line of text.  Scroll text up one line if needed.
{
	ASSERT(this->texts.size() == this->wDisplayLines);
	if (!WCSlen(pwczSetText)) return;

	//Find whether there are any empty lines to place text.
	UINT wIndex;
	for (wIndex=0; wIndex<this->wDisplayLines; ++wIndex)
		if (this->texts[wIndex].text.empty())
			break;

	if (wIndex == this->wDisplayLines)
	{
		//No room -- Scroll existing lines up to make room at bottom.
		for (wIndex=0; wIndex<this->texts.size()-1; ++wIndex)
			this->texts[wIndex] = this->texts[wIndex+1];
	}
	this->texts[wIndex].text = pwczSetText;
	this->texts[wIndex].color = color;

	this->dwDuration = dwDuration;

	const UINT prevH = this->h;

	PrepWidget();

	if (bSyncTextOffsetUpward) {
		//keep lower edge of effect at a constant position as new lines of text are added
		const int deltaH = int(this->h) - int(prevH);
		if (deltaH != 0) {
			this->yOffset -= deltaH;
			//Snap effect to this offset.
			this->x = this->y = (UINT)-1;
		}
	}
}

//*****************************************************************************
void CSubtitleEffect::SetToText(const WCHAR* pwczSetText, const Uint32 dwDuration, const SDL_Color& color) //[default=Black]
//Sets displayed text to indicated text.
{
	this->texts[0].text = pwczSetText ? pwczSetText : wszEmpty;
	this->texts[0].color = color;
	for (UINT wIndex=1; wIndex<this->texts.size()-1; ++wIndex)
	{
		this->texts[wIndex].text = wszEmpty;
		this->texts[wIndex].color = Black;
	}

	this->dwDuration = dwDuration;

	PrepWidget();
}


//*****************************************************************************
bool CSubtitleEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Exit when there's no text to be drawn.
	if (!this->texts.size()) return false;
	if (this->texts[0].text.empty())
		return false;

	if (this->dwDuration && dwTimeElapsed > this->dwDuration + dwWaitForNewText)
	{
		//Start fading out text.
		if (dwTimeElapsed >= this->dwDuration + this->dwFadeDuration)
		{
			RemoveFromSubtitles();
			return false;
		}
		if (g_pTheBM->bAlpha)
		{
			const float fFadePerMS = this->opacity / float(this->dwFadeDuration);
			const Uint8 opacity = this->opacity - (Uint8)((dwTimeElapsed - this->dwDuration) * fFadePerMS);
			ASSERT(opacity <= this->opacity);
			EnableSurfaceBlending(this->pTextSurface, opacity);
		}
	}

	SetLocation();

	return true;
}

//*****************************************************************************
void CSubtitleEffect::Draw(SDL_Surface& pDestSurface)
{
	SDL_Rect SrcRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	SDL_Rect ScreenRect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	SDL_BlitSurface(this->pTextSurface, &SrcRect, &pDestSurface, &ScreenRect);
}

//*****************************************************************************
Uint32 CSubtitleEffect::GetDisplayTimeRemaining() const
//Returns: time (ms) until effect begins to fade and no more text may be added to it
{
	if (!this->dwDuration)
		return Uint32(-1); //effect is displaying indefinitely

	return this->dwDuration + dwWaitForNewText - this->dwTimeElapsed;
}

//*****************************************************************************
void CSubtitleEffect::SetLocation()
{
	//Center effect on coord's position.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);

	const int nCX = this->xOffset +
			(this->pCoord ? this->pCoord->wX * CBitmapManager::CX_TILE : 0);
	const int nCY = this->yOffset +
			(this->pCoord ? this->pCoord->wY * CBitmapManager::CY_TILE : 0);
	const int wNewX = OwnerRect.x + nCX;
	const int wNewY = OwnerRect.y + nCY;

	if (this->x == static_cast<UINT>(-1))
	{
		//Start at this position.
		this->x = wNewX;
		this->y = wNewY;
	} else {
		static const int UPDATE_FACTOR = 10;

		//If/as coord moves, have effect pursue it.
		//Only pursue coordinates within the owner widget.
		if (OwnerRect.x <= wNewX && wNewX <= OwnerRect.x + OwnerRect.w)
		{
			int dx = wNewX - this->x;
			if (abs(dx) >= UPDATE_FACTOR)
				dx /= UPDATE_FACTOR;
			else dx = sgn(dx);

			this->x += dx;
		}
		if (OwnerRect.y <= wNewY && wNewY <= OwnerRect.y + OwnerRect.h)
		{
			int dy = wNewY - this->y;
			if (abs(dy) >= UPDATE_FACTOR)
				dy /= UPDATE_FACTOR;
			else dy = sgn(dy);
			
			this->y += dy;
		}
	}

	//Coordinates should always be situated within parent widget.
//might not be true for global NPC entities speaking from outside the room coords
//	ASSERT(nCX - this->xOffset < int(OwnerRect.w));
//	ASSERT(nCY - this->yOffset < int(OwnerRect.h));

	//Keep within parent widget.
	if (static_cast<int>(this->x) < OwnerRect.x)
		this->x = OwnerRect.x;
	else if (this->x + this->w >= static_cast<UINT>(OwnerRect.x + OwnerRect.w))
		this->x = OwnerRect.x + OwnerRect.w - this->w;
	if (static_cast<int>(this->y) < OwnerRect.y)
		this->y = OwnerRect.y;
	else if (this->y + this->h >= static_cast<UINT>(OwnerRect.y + OwnerRect.h))
		this->y = OwnerRect.y + OwnerRect.h - this->h;

	//Update area of effect.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->x;
	this->dirtyRects[0].y = this->y;
}
