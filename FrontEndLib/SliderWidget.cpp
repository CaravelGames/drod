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

#include "SliderWidget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Exception.h>

#include <algorithm>

//Source coords and dimensions within parts surface.
const int X_SLIDER = 294;
const int Y_SLIDER = 46;
const UINT CX_SLIDER = 14;
const UINT CY_SLIDER = 28;

//
//Public methods.
//

//******************************************************************************
CSliderWidget::CSliderWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,      //    constructor.
	const UINT wSetW, const UINT wSetH,    //
	const BYTE bytSetValue,                //(in)   0 = left, 255 = right.
	const BYTE bytNumTicks)                //(in) # of tick increments [default = 0:off]
	: CFocusWidget(WT_Slider, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bytValue(bytSetValue), bytPrevValue(bytSetValue)
	, bytTickMarks(bytNumTicks)
	, bWasSliderDrawn(false)
	, bFocusRegionsSaved(false)
	, pEraseSurface(NULL)
	, bDrawTickMarks(false)
{
	this->pBiggerTicks = std::vector<UINT>();
	this->imageFilenames.push_back(string("Dialog"));

	this->pFocusSurface[0] = this->pFocusSurface[1] = NULL;

	//Create surface to save screen surface bits of an area before slider
	//is blitted to it.
	this->pEraseSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, CX_SLIDER, CY_SLIDER, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pEraseSurface) throw CException();

	//Create surface to save screen surface bits where focus is drawn.
	this->pFocusSurface[0] = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w-2, 1, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pFocusSurface[0]) throw CException();
	this->pFocusSurface[1] = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w-2, 1, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pFocusSurface[1]) throw CException();

	//Bounds check.
	if (this->bytTickMarks == 1) this->bytTickMarks = 0;	//1 tick is meaningless
	if (this->bytTickMarks && bytSetValue >= this->bytTickMarks)
		this->bytValue = this->bytPrevValue = this->bytTickMarks-1;
}

//******************************************************************************
CSliderWidget::~CSliderWidget()
{
	if (this->pEraseSurface)
	{
		SDL_FreeSurface(this->pEraseSurface);
		this->pEraseSurface = NULL;
	}

	if (this->pFocusSurface[0])
	{
		SDL_FreeSurface(this->pFocusSurface[0]);
		this->pFocusSurface[0] = NULL;
	}
	if (this->pFocusSurface[1])
	{
		SDL_FreeSurface(this->pFocusSurface[1]);
		this->pFocusSurface[1] = NULL;
	}
}

//******************************************************************************
void CSliderWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	ASSERT(static_cast<UINT>(this->w) >= CX_SLIDER);
	ASSERT(static_cast<UINT>(this->y) >= CX_SLIDER);

	if (!IsVisible(true))
		return; // Invisible widgets should not be drawn

	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	static SDL_Rect SliderRect = MAKE_SDL_RECT(X_SLIDER, Y_SLIDER, CX_SLIDER, CY_SLIDER);
	static SDL_Rect EraseRect = MAKE_SDL_RECT(0, 0, CX_SLIDER, CY_SLIDER);
	static UINT TickMarkSmall = 7;
	static UINT TickMarkBig = 17;
	SDL_Rect ScreenRect = MAKE_SDL_RECT(0, 0, CX_SLIDER, CY_SLIDER);
	SDL_Rect FocusScreenRect = MAKE_SDL_RECT(this->x + 1, 0, this->w-2, 1);
	SDL_Rect FocusRect = MAKE_SDL_RECT(0, 0, this->w-2, 1);

	//Calc surface colors if needed.
	SDL_Surface *pDestSurface = GetDestSurface();
	const SURFACECOLOR Light = GetSurfaceColor(pDestSurface, 225, 214, 192), 
			Dark = GetSurfaceColor(pDestSurface, 145, 132, 109);
	
	//Save spots where focus lines are drawn.
	if (!this->bFocusRegionsSaved)
	{
		FocusScreenRect.y = this->y;
		SDL_BlitSurface(pDestSurface, &FocusScreenRect,
				this->pFocusSurface[0], &FocusRect);
		FocusScreenRect.y = this->y + this->h - 1;
		SDL_BlitSurface(pDestSurface, &FocusScreenRect,
				this->pFocusSurface[1], &FocusRect);
		this->bFocusRegionsSaved = true;
	}

	//Erase slider at previous location.
	ScreenRect.y = this->y + (this->h - CY_SLIDER) / 2;
	if (this->bWasSliderDrawn)
	{
		ScreenRect.x = static_cast<Sint16>( this->x + 1 +
				((double) this->bytPrevValue / 255.0) * (this->w - CX_SLIDER - 2) );
		SDL_BlitSurface(this->pEraseSurface, &EraseRect,
				pDestSurface, &ScreenRect);
	}

	//Draw the line that slider travels over.
	int yLine = this->y + this->h / 2;
	DrawCol(this->x + 1, this->y, this->h, Light);
	DrawCol(this->x + this->w - 1, this->y, this->h, Light);
	DrawRow(this->x + 1, yLine, this->w - 2, Light);

	// <X position of tick, height of tick>
	std::vector< std::pair<UINT, UINT> > ticks;
	ticks.reserve(this->bytTickMarks);

	if (this->bDrawTickMarks && this->bytTickMarks > 0) {
		for (BYTE i = 0; i < this->bytTickMarks; i += 1) {
			ticks.push_back(std::make_pair(
				this->x + (this->w * i / this->bytTickMarks) + SliderRect.w / 2,
				std::find(this->pBiggerTicks.begin(), this->pBiggerTicks.end(), i) != this->pBiggerTicks.end()
				? TickMarkBig : TickMarkSmall
			));
		}
	}

	for (BYTE i = 0; i < ticks.size(); i++) {
		std::pair<UINT, UINT> lineData = ticks[i];
		DrawCol(lineData.first + 1, this->y + (this->h - lineData.second) / 2, lineData.second, Light);
	}

	DrawCol(this->x, this->y, this->h, Dark);
	DrawCol(this->x + this->w - 2, this->y, this->h, Dark);
	DrawRow(this->x + 1, yLine - 1, this->w - 2, Dark);

	for (BYTE i = 0; i < ticks.size(); i++) {
		std::pair<UINT, UINT> lineData = ticks[i];
		DrawCol(lineData.first, this->y + (this->h - lineData.second) / 2, lineData.second, Dark);
	}

	//Calculate where to put slider.
	float fSlider;
	if (!this->bytTickMarks)
		fSlider = (float)this->bytValue;
	else
	{
		//Spread tick increments over range.
		ASSERT(this->bytValue < this->bytTickMarks);
		const float fTickIncWidth = 255.0f / (float)(this->bytTickMarks-1);
		fSlider = this->bytValue * fTickIncWidth;
	}
	ASSERT(fSlider >= 0);
	ASSERT(fSlider <= 255);

	//Copy underneath slider area to surface that can be used to erase it later.
	this->bytPrevValue = (BYTE)fSlider;   //save position thumb is drawn at
	ScreenRect.x = static_cast<Sint16>( this->x + 1 +
			((float)this->bytPrevValue / 255.0) * (this->w - CX_SLIDER - 2) );
	SDL_BlitSurface(pDestSurface, &ScreenRect, this->pEraseSurface, &EraseRect);

	//Draw the slider in its current location.
	SDL_BlitSurface(this->images[0], &SliderRect, pDestSurface, &ScreenRect);
	this->bWasSliderDrawn = true;

	PaintChildren();

	if (IsSelected())
		DrawFocused();
	else
	{
		//erase focus lines
		FocusScreenRect.y = this->y;
		SDL_BlitSurface(this->pFocusSurface[0], &FocusRect, pDestSurface, &FocusScreenRect);
		FocusScreenRect.y = this->y + this->h - 1;
		SDL_BlitSurface(this->pFocusSurface[1], &FocusRect, pDestSurface, &FocusScreenRect);
	}

	if (bUpdateRect) UpdateRect();
}
//******************************************************************************
void CSliderWidget::SetDrawTickMarks(const bool bDrawTickMarks)
{
	this->bDrawTickMarks = bDrawTickMarks;
}

//******************************************************************************
void CSliderWidget::SetValue(
//Set value of slider, which affects its position.
//
//Params:
	const BYTE bytSetValue) //(in)   New value.
{
	const UINT oldValue = this->bytValue;
	this->bytValue = (this->bytTickMarks && bytSetValue >= this->bytTickMarks) ?
		this->bytTickMarks-1 : bytSetValue;

	if (this->bytValue != oldValue)
	{
		//Call OnSelectChange() notifier.
		CEventHandlerWidget* pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}

	RequestPaint();
}

//
//Protected methods.
//

//******************************************************************************
void CSliderWidget::HandleDrag(
//Handle mouse dragging of the slider.
//
//Params:
	const SDL_MouseMotionEvent &Motion) //(in)   Event to handle.
{
	SetToX(Motion.x);
	RequestPaint();
}

//******************************************************************************
void CSliderWidget::HandleMouseDown(
//Handle mousedown on the slider.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in)   Event to handle.
{
	SetToX(Button.x);
	RequestPaint();
}

//******************************************************************************
void CSliderWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
//
//Returns:
//True.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;
	const BYTE incAmount = this->bytTickMarks ? (BYTE)1 : (BYTE)8;
	const BYTE bytMaxValue = this->bytTickMarks ? this->bytTickMarks - 1 : 255;

	switch (key)
	{
		case SDLK_HOME:  case SDLK_KP_7:
			if (this->bytValue > 0)
				SetValue(0);
		break;
		case SDLK_LEFT: case SDLK_KP_4:
			//slide left
			if (this->bytValue > 0)
			{
				if (KeyboardEvent.keysym.mod & KMOD_CTRL)
				{  //all the way to left
					SetValue(0);
				} else {
					if (this->bytValue < incAmount)
						SetValue(0);
					else
						SetValue(this->bytValue - incAmount);
				}
			}
		break;

		case SDLK_RIGHT: case SDLK_KP_6:
			//slide right
			if (this->bytValue < bytMaxValue)
			{
				if (KeyboardEvent.keysym.mod & KMOD_CTRL)
				{  //all the way to right
					SetValue(bytMaxValue);
				} else {
					if (this->bytValue > bytMaxValue - incAmount)
						SetValue(bytMaxValue);
					else
						SetValue(this->bytValue + incAmount);
				}
			}
		break;
		case SDLK_END: case SDLK_KP_1:
			SetValue(bytMaxValue);
		break;
		default: break;
	}
}

//
//Private methods.
//

//******************************************************************************
void CSliderWidget::DrawFocused()
//Draw focus.
{
	SDL_Surface *pDestSurface = GetDestSurface();
	const SURFACECOLOR FocusColor = GetSurfaceColor(pDestSurface, RGB_FOCUS);
	const SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);

	DrawRect(rect,FocusColor,pDestSurface);
}

//******************************************************************************
void CSliderWidget::SetToX(
//Set value of slider based on x coordinate on screen.
//
//Params:
	const int nX)  //(in)   x coordinate.
{
	const BYTE oldValue = this->bytValue;

	if (nX < static_cast<int>(this->x))
		SetValue(0);
	else if (nX >= static_cast<int>(this->x + this->w - CX_SLIDER))
		SetValue(this->bytTickMarks ? this->bytTickMarks-1 : 255);
	else
	{
		BYTE bytNewValue = static_cast<BYTE>( (double)(nX - this->x) /
				(double)(this->w - CX_SLIDER) * 255.0 );
		if (this->bytTickMarks)
		{
			//Snap slider thumb to a tick increment.
			const float fTickIncWidth = 256.0f / (float)(this->bytTickMarks-1);
			float value = bytNewValue;
			bytNewValue = (BYTE)(value / fTickIncWidth);
			value -= bytNewValue * fTickIncWidth;
			const bool bRoundUp = (BYTE(value) % ((UINT)fTickIncWidth + 1)) >= (fTickIncWidth / 2.0f);
			if (bRoundUp)
				++bytNewValue;
		}
		SetValue(bytNewValue);
	}

	if (this->bytValue != oldValue)
	{
		//Call OnSelectChange() notifier.
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}
}
