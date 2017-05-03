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
 * Contributor(s): Mike Rimer (mrimer), Matt Schikore (Schik)
 *
 * ***** END LICENSE BLOCK ***** */

#include "ClockWidget.h"
#include "DrodBitmapManager.h"

#include "../DRODLib/SettingsKeys.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>

#include <limits>

//
//Public methods.
//

//******************************************************************************
CClockWidget::CClockWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,               //    constructor.
	const UINT wSetW, const UINT wSetH)             //
	: CWidget((WIDGETTYPE)WT_Clock, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, pEraseSurface(NULL)
	, bEraseSurfaceMade(false)
	, wTime(0)
	, bHalfStep(false)
	, bClockDrawn(false), bEraseOldClock(false)
	, bClockVisible(false)
	, wPixelsShowing(0)
	, dwLastAnimate(0)
	, bShowImmediate(false)
{
   CFiles::GetGameProfileString(INISection::Graphics, "Clock", clockOptions);

   if (clockOptions.size() == 0)
      clockOptions.push_back("Clock");
   this->curOption = clockOptions.begin();
   this->imageFilenames.push_back(*this->curOption + string("2"));
   this->imageFilenames.push_back(*this->curOption + string("1"));

	this->pEraseSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, CX_CLOCK, CY_CLOCK, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pEraseSurface) throw CException();
}

//******************************************************************************
CClockWidget::~CClockWidget()
{
	if (this->pEraseSurface)
	{
		SDL_FreeSurface(this->pEraseSurface);
		this->pEraseSurface = NULL;
	}
}

//******************************************************************************
void CClockWidget::SetTime(const UINT wValue, const bool bHalfStep)
{
	this->wTime = wValue;
	this->bHalfStep = bHalfStep;
	this->bClockDrawn = false;
}

//******************************************************************************
bool bIsClockVisible(CClockWidget::ClockState state)
{
	return state != CClockWidget::CS_Hide;
}

bool bDisplayHalfStep(CClockWidget::ClockState state, bool gameVal)
{
	if (state >= CClockWidget::CS_ExplicitTimeStart && state < CClockWidget::CS_ExplicitTimeEnd)
		return false;
	if (state >= CClockWidget::CS_ExplicitHalfTimeStart && state < CClockWidget::CS_ExplicitHalfTimeEnd)
		return true;
	return gameVal;
}

UINT calcDisplayTime(CClockWidget::ClockState state, UINT wGameTime, bool halfStep)
{
	if (state >= CClockWidget::CS_ExplicitTimeStart && state < CClockWidget::CS_ExplicitTimeEnd)
		return UINT(state) - CClockWidget::CS_ExplicitTimeStart;
	if (state >= CClockWidget::CS_ExplicitHalfTimeStart && state < CClockWidget::CS_ExplicitHalfTimeEnd)
		return UINT(state) - CClockWidget::CS_ExplicitHalfTimeStart;
	if (state == CClockWidget::CS_ShowBackwards) {
		static const int maxval = (std::numeric_limits<int>::max() / TIME_INCREMENTS) * TIME_INCREMENTS;
		wGameTime = static_cast<UINT>(maxval - int(wGameTime));
		if (halfStep)
			--wGameTime; //show half step backwards
	}
	return wGameTime;
}

//******************************************************************************
void CClockWidget::ShowClock(
//Make clock visible/hidden and set time.  Repaint.
//
//Params:
	const UINT wTime,		//time to display on clock
	const bool bHalfStep,//show halfway between increments
	const ClockState state,	//whether to show or hide clock, etc.
	const bool bReset)	//[default=false] if true, shows or hides clock immediately
{
	Show();
	this->bShowImmediate = bReset;

	const bool displayHalfStep = bDisplayHalfStep(state, bHalfStep);
	const UINT displayTime = calcDisplayTime(state, wTime, displayHalfStep);
	SetTime(displayTime, displayHalfStep);

	const bool bVisible = bIsClockVisible(state);
	if (bVisible == this->bClockVisible) {
		RequestPaint();   //Show new time.
	} else {
		//Start animating show/hide.
		this->bClockVisible = bVisible;
		this->dwLastAnimate = SDL_GetTicks();
	}
}

//******************************************************************************
void CClockWidget::HandleAnimate()
//Handle animation of the widget.
//Specifically, sliding on and off the screen.
{
	//When showing/hiding the clock, slide it on/off the screen.
	static const Uint32 fMSPerPixel = 1000 / CX_CLOCK; //1 second animation

	const Uint32 dwNow = SDL_GetTicks();
	UINT wPixelsToSlide = (dwNow - this->dwLastAnimate) / fMSPerPixel;
	if (this->bShowImmediate)
	{
		//Don't slide the clock in/out of view.  Show it immediately.
		wPixelsToSlide = CX_CLOCK;
		this->bShowImmediate = false;
	}
	if (!wPixelsToSlide)
		return;

	//Slide clock on/off screen.
	if (this->bClockVisible && this->wPixelsShowing < CX_CLOCK)
	{
		if (!this->wPixelsShowing)
		{
			//Clock is going on-screen.  Save area underneath to erase clock later.
			if (!this->bEraseSurfaceMade)
			{
				//Mark transparent color on clock parts surface once.
				static Uint32 TransparentColor = SDL_MapRGB(this->images[0]->format, TRANSPARENT_RGB);
				SetColorKey(this->images[0], SDL_TRUE, TransparentColor);
				static Uint32 TransparentColor2 = SDL_MapRGB(this->images[1]->format, TRANSPARENT_RGB);
				SetColorKey(this->images[1], SDL_TRUE, TransparentColor2);
			}
			SDL_Rect ScreenRect = MAKE_SDL_RECT(this->x, this->y, CX_CLOCK, CY_CLOCK);
			SDL_Rect EraseRect = MAKE_SDL_RECT(0, 0, CX_CLOCK, CY_CLOCK);
	      SDL_BlitSurface(GetDestSurface(), &ScreenRect, this->pEraseSurface, &EraseRect);
			this->bEraseSurfaceMade = true;
		}

		//Slide on.
		if (wPixelsToSlide > CX_CLOCK - this->wPixelsShowing)
			wPixelsToSlide = CX_CLOCK - this->wPixelsShowing;

		this->wPixelsShowing += wPixelsToSlide;
		this->bEraseOldClock = true;
	}
	else if (!this->bClockVisible && this->wPixelsShowing > 0)
	{
		//Slide off.
		if (wPixelsToSlide > this->wPixelsShowing)
			wPixelsToSlide = this->wPixelsShowing;

		this->wPixelsShowing -= wPixelsToSlide;
		this->bEraseOldClock = true;
	}
	else return;   //don't need to redraw anything

	this->dwLastAnimate += wPixelsToSlide * fMSPerPixel;
	RequestPaint();
}

//******************************************************************************
void CClockWidget::Paint(
//Paint the clock.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	//Make things easy and insist on standard width and height.
	ASSERT(this->w == CX_CLOCK && this->h == CY_CLOCK);

	if (!this->bEraseSurfaceMade)
		return;  //wait until the background has been saved

	//Draw clock.
	SDL_Surface *pDestSurface = GetDestSurface();
	if (this->bEraseOldClock)
	{
		//Erase old clock.
		SDL_Rect EraseRect = MAKE_SDL_RECT(0, 0, CX_CLOCK, CY_CLOCK);
		SDL_Rect ScreenRect = MAKE_SDL_RECT(this->x, this->y, CX_CLOCK, CY_CLOCK);
		SDL_BlitSurface(this->pEraseSurface, &EraseRect, pDestSurface, &ScreenRect);
	}
	if (!this->bClockDrawn || this->bEraseOldClock)
	{
		//Draw clock.
		SDL_Rect Src = MAKE_SDL_RECT(CX_CLOCK - this->wPixelsShowing, CY_CLOCK * (this->wTime%TIME_INCREMENTS),
				this->wPixelsShowing, CY_CLOCK);
		SDL_Rect Dest = MAKE_SDL_RECT(this->x, this->y, this->wPixelsShowing, CY_CLOCK);
		SDL_BlitSurface(this->images[this->bHalfStep ? 1 : 0], &Src, pDestSurface, &Dest);
		this->bClockDrawn = true;
	}

	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CClockWidget::NextOption()
//Change the graphical representation of the clock to the next available one.
{
	ASSERT(!this->clockOptions.empty());
   if (++this->curOption == this->clockOptions.end())
      this->curOption = this->clockOptions.begin();

   Unload();
   this->imageFilenames.clear();
   this->imageFilenames.push_back(*this->curOption + string("2"));
   this->imageFilenames.push_back(*this->curOption + string("1"));
   Load();
   Paint();
}

//******************************************************************************
void CClockWidget::HandleMouseDown(
//Called when the mouse is clicked on the clock.  For now, it will simply
//advance the clock to the next style option specified in the INI file.
//
//Params:
   const SDL_MouseButtonEvent &Button) //(in) Event to handle.
{
   if (Button.button == SDL_BUTTON_LEFT)
		NextOption();
}
