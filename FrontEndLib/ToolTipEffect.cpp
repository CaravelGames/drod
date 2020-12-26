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

#include "ToolTipEffect.h"
#include "BitmapManager.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>

//Background tool tip color.
#ifdef GAME_RENDERING_OFFSET
const SURFACECOLOR BGColor = {255, 250, 205};   //pale yellow
#else
const SURFACECOLOR BGColor = {205, 250, 255};   //pale yellow
#endif
const SURFACECOLOR Border = {0, 0, 0}; //black

const UINT  wBorder = 2;
//
//Public methods.
//

//*****************************************************************************
CToolTipEffect::CToolTipEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,                //(in)   Required params for CEffect 
	const CCoord &SetCoord,             //(in)   Desired location of text.
	const WCHAR *pwczSetText,           //(in)   Text that label will display.
	const Uint32 dwDisplayDuration,     //(in)   Display (default = 5000ms)
	const UINT eSetFontType)
	: CEffect(pSetWidget, dwDisplayDuration, EFFECTLIB::ETOOLTIP)
	, eFontType(eSetFontType)
	, pToolTipSurface(NULL)
{
	this->wstrText = pwczSetText ? pwczSetText : wszEmpty;

	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);

	UINT wW, wH;
	this->w = OwnerRect.w;
	GetTextWidthHeight(wW,wH);
	this->w = wW + wBorder*2 + 2;
	this->h = wH + wBorder*2 + 2;

	this->x = SetCoord.wX;
	this->y = SetCoord.wY;
	if (static_cast<Sint16>(this->y) - static_cast<Sint16>(this->h) >= OwnerRect.y)
		this->y -= this->h;  //show tool tip just above mouse cursor
	else
		this->y += this->h;  //no room -- show tool tip just below mouse cursor

	//Move away from edge of parent widget, if necessary.
	if (this->x + this->w >= static_cast<UINT>(OwnerRect.x + OwnerRect.w))
		this->x = OwnerRect.x + OwnerRect.w - this->w;
	if (this->y + this->h >= static_cast<UINT>(OwnerRect.y + OwnerRect.h))
		this->y = OwnerRect.y + OwnerRect.h - this->h;

	//Area of effect.
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	this->dirtyRects.push_back(rect);

	//Render tool tip to internal surface (to avoid re-rendering each frame).
	this->pToolTipSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	ASSERT(this->pToolTipSurface);
	SDL_Rect TextRect = MAKE_SDL_RECT(1, 1, this->w-2, this->h-2);
	SDL_Rect BorderRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	//Fill rectangle and draw border.
	this->pOwnerWidget->DrawFilledRect(BorderRect, BGColor, pToolTipSurface);
	this->pOwnerWidget->DrawRect(BorderRect, Border, pToolTipSurface);
#ifdef RUSSIAN_BUILD
	static const int YOFFSET = 0;
#else
	static const int YOFFSET = 3;   //this font always draws too low
#endif

	g_pTheFM->DrawTextToRect(this->eFontType, this->wstrText.c_str(), 
			TextRect.x+wBorder, TextRect.y - YOFFSET, TextRect.w-wBorder,
			TextRect.h-wBorder, pToolTipSurface);
}

//*****************************************************************************
CToolTipEffect::~CToolTipEffect()
{
	SDL_FreeSurface(this->pToolTipSurface);
}

//*****************************************************************************
void CToolTipEffect::GetTextWidthHeight(
//Gets width and height of text as it is drawn within label.
//
//Params:
	UINT &wW, UINT &wH)  //(out) Width and height of text.
const
{
	if (this->wstrText.size()==0) 
	{
		wW = wH = 0;
		return;
	}

	//Ask font manager about it.
	g_pTheFM->GetTextRectHeight(this->eFontType, this->wstrText.c_str(), 
			this->w, wW, wH);
}

//*****************************************************************************
bool CToolTipEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (this->wstrText.size() == 0) 
		return false;

	return true;
}

//*****************************************************************************
void CToolTipEffect::Draw(SDL_Surface& destSurface)
{
	SDL_Rect ToolTipRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	SDL_Rect ScreenRect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	SDL_BlitSurface(this->pToolTipSurface, &ToolTipRect, &destSurface, &ScreenRect);
}
