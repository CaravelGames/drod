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

#include "ScrollingTextWidget.h"

#include <BackEndLib/Assert.h>

#include <SDL_ttf.h>

//
//Public methods.
//

//******************************************************************************
CScrollingTextWidget::CScrollingTextWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget 
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH,             //
	const UINT wPixelsPerSecond)     //(in)   Scroll rate.
	: CWidget(WT_ScrollingText, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, pBackgroundSurface(NULL)
	, bPaused(false)
{
	SetScrollRate(wPixelsPerSecond);
	ClearText();
}

//******************************************************************************
void CScrollingTextWidget::ClearText()
//Erases all text.
{
	ClearChildren();
	ScrollAbsolute(0, 0);
	this->nNextLabelY = this->h;
	this->dwLastScroll = 0L;
}

//******************************************************************************
bool CScrollingTextWidget::IsAllTextShown() const
//Has all text been shown inside of the widget?  If there is still text waiting
//below the widget's scrolled view then the answer is "no".
//
//Returns:
//True if all text has been shown, false if not.
{
	//Amount of space to have below text before this returns true.
	static const UINT BUFFER_PIXELS = 15;

	if (empty())
		//Either no text has been added, or it has all scrolled past the top.
		return true;

	//Get the lowest text label.  It will be the last child of this widget
	//because of how AddText() works.
	WIDGET_ITERATOR iLowestLabel = this->Children.end();
	--iLowestLabel;
	CLabelWidget *pLowestLabel = DYN_CAST(CLabelWidget*, CWidget*, *iLowestLabel);

	return (pLowestLabel->GetY() + (int) pLowestLabel->GetH() +
			this->nChildrenScrollOffsetY < static_cast<int>(this->y + this->h - BUFFER_PIXELS));
}

//******************************************************************************
void CScrollingTextWidget::HandleAnimate()
{
	if (!empty())
	{
		ScrollText();
		RequestPaint();
	}
}

//******************************************************************************
void CScrollingTextWidget::Paint(
//Paint text inside of the widget area.
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

	SDL_Surface *pDestSurface = GetDestSurface();

	if (this->pBackgroundSurface)
	{
		SDL_Rect BgSrc = MAKE_SDL_RECT(this->nBackgroundX, this->nBackgroundY, this->w, this->h);
		SDL_Rect BgDest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		if (BgSrc.x == -1) BgSrc.x = this->x;
		if (BgSrc.y == -1) BgSrc.y = this->y;
		SDL_BlitSurface(this->pBackgroundSurface, &BgSrc, pDestSurface, &BgDest);
	}

	PaintChildren();

	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CScrollingTextWidget::Add(CWidget *pWidget)
//Add a new widget to the end of the list.
{
	if (!pWidget) return;

	//If the last label is showing on the widget, put the next label just past
	//the bottom of the widget area, instead of right underneath the
	//already-showing last label.  It looks a little jarring to have it pop up
	//out of nowhere.
	if (
			//Cheap check--scrolling must occur before above condition can be true.
			this->nChildrenScrollOffsetY != 0 &&

			//The last label has been completely shown but has not scrolled off the
			//top yet.
			!empty() && IsAllTextShown())
		this->nNextLabelY = (int) this->h - this->nChildrenScrollOffsetY;

	pWidget->Move(pWidget->GetX(), this->nNextLabelY);
	AddWidget(pWidget);

	//Height was resized to fit the text.  Increment nNextLabelY so
	//the next item will go underneath this widget.
	this->nNextLabelY += pWidget->GetH();
}

//******************************************************************************
void CScrollingTextWidget::AddText(
//Add new text item to the end of the list.
//
//Params:
	const WCHAR *pwczText,              //(in)
	const UINT eFontType,               //(in)
	CLabelWidget::TEXTALIGN eTextAlign) //(in)
{
	//If the last label is showing on the widget, put the next label just past
	//the bottom of the widget area, instead of right underneath the
	//already-showing last label.  It looks a little jarring to have it pop up
	//out of nowhere.
	if (
			//Cheap check--scrolling must occur before above condition can be true.
			this->nChildrenScrollOffsetY != 0 &&

			//The last label has been completely shown but has not scrolled off the
			//top yet.
			!empty() && IsAllTextShown())
		this->nNextLabelY = (int) this->h - this->nChildrenScrollOffsetY;

	//Add a new last label underneath any existing labels.
	CLabelWidget *pLabel = new CLabelWidget(0, 0, this->nNextLabelY,
			this->w, this->h, eFontType, pwczText, true);
	pLabel->SetAlign(eTextAlign);
	AddWidget(pLabel, true);

	//Height of label was resized to fit the text.  Increment nNextLabelY so
	//the next label will go underneath this label.
	this->nNextLabelY += pLabel->GetH();
}

//******************************************************************************
CWidget* CScrollingTextWidget::GetLastShownWidget() const
//Gets the last (lowest) item in list that is currently shown inside the widget.
//
//Returns:
//Pointer to child widget or NULL if no item is showing.
{
	if (!this->Children.size()) return NULL; //No labels.

	//Looking at items from last to first.  Items are ordered by their
	//y coord because of how AddText() works.
	WIDGET_ITERATOR iSeek = this->Children.end();
	while (iSeek != this->Children.begin())
	{
		--iSeek;
		if ((*iSeek)->GetY() + this->nChildrenScrollOffsetY < static_cast<int>(this->y + this->h))
			//Found the lowest showing label.
			return *iSeek;
	}

	//No items are showing.
	return NULL;
}

//******************************************************************************
void CScrollingTextWidget::SetBackground(
//Sets background image that goes underneath text.
//
//Params:
	SDL_Surface *pSetSurface,  //(in)   Pointer to source surface used for future
								//    blits.  Caller must make sure the surface
								//    is available for life of this object or
								//    make another call to SetBackground() when
								//    the surface becomes unavailable.  Pass NULL
								//    to specify that no background surface be
								//    used.
	const int nSetX,        //(in)   Source coords to blit from source surface.
	const int nSetY)        //    If -1 (default), then the coords of this 
								//    widget will be used.
{
	//Make sure blit coords are not out-of-bounds.
	ASSERT(nSetX >= -1 && static_cast<int>(nSetX + this->w) < pSetSurface->w);
	ASSERT(nSetY >= -1 && static_cast<int>(nSetY + this->h) < pSetSurface->h);

	this->pBackgroundSurface = pSetSurface;
	this->nBackgroundX = nSetX;
	this->nBackgroundY = nSetY;
}

//******************************************************************************
void CScrollingTextWidget::ScrollText()
//Scroll children up according to how much time has passed since last scrolling.
{
	if (!this->Children.size()) return;
	if (this->bPaused) return;

	//Determine number of pixels to scroll up.
	const UINT dwNow = SDL_GetTicks();
	if (!this->dwLastScroll)
		this->dwLastScroll = SDL_GetTicks();
	const int nPixels = (this->wMSPerPixel == 0) ? 0 :
			(dwNow - this->dwLastScroll) / this->wMSPerPixel;
	if (nPixels)
	{
		Scroll(0, -nPixels);
		this->dwLastScroll += nPixels * this->wMSPerPixel;
	}

	//Remove any items that have scrolled off the top.
	WIDGET_ITERATOR iSeek = this->Children.begin();
	while (iSeek != this->Children.end())
	{
		WIDGET_ITERATOR i = iSeek;
		++iSeek;
		if ((*i)->GetY() + (int) (*i)->GetH() + this->nChildrenScrollOffsetY < 
				this->y)
			RemoveWidget(*i);
	}

	if (empty())
	{
		//Reset scrolling settings.
		ClearText();
	}
}

//******************************************************************************
void CScrollingTextWidget::SetScrollRate(
//Sets scrolling text rate.
//A value of 0 pauses scrolling.
//
//Params:
	const UINT wPixelsPerSecond)
{
	if (wPixelsPerSecond == 0)
	{
		this->bPaused = true;
		return;
	}
	if (this->bPaused)
	{
		this->bPaused = false;
		this->dwLastScroll = 0L;   //reset after pause
	}
	if (wPixelsPerSecond)
		this->wMSPerPixel = 1000 / wPixelsPerSecond;
	else
		this->wMSPerPixel = 1000;
}
