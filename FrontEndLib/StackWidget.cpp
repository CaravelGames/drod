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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "StackWidget.h"
#include "BitmapManager.h"
#include "Inset.h"
#include <BackEndLib/Assert.h>
#include "EventHandlerWidget.h"

//
//Public methods.
//

//*********************************************************************************************************
CStackWidget::CStackWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget
	int nSetX, int nSetY,               //    constructor.
	int nMaxSize,
	int nPadding,
	bool bVertical)
	: CFocusWidget(WT_Stack, dwSetTagNo, nSetX, nSetY, 0, 0)
	, bIsVertical(bVertical)
	, nMaxSize(nMaxSize)
	, nTotalSize(0)
	, nPadding(nPadding)
	, nCurOffset(0)
{
}

//*********************************************************************************************************
void CStackWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//TODO:  If there is padding between elements, need to redraw the background.

	SDL_Surface *pDestSurface = GetDestSurface();
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
	const bool bParentIsNotStack = dynamic_cast<CStackWidget*>(this->pParent) == NULL;
	if (bParentIsNotStack) {
		// When using nested stacks, we want to clip only based on the topmost one.
		SDL_SetClipRect(pDestSurface, &rect);
	}
	int drawX = this->x;
	int drawY = this->y;
	int newX, newY;
	for (vector<CWidget*>::const_iterator it = this->widgets.begin(); it != this->widgets.end(); it++) {
		CWidget* pWidget = *it;
		newX = drawX;
		newY = drawY;
		if (this->nCurOffset) {
			if (this->bIsVertical) {
				newY -= this->nCurOffset;
			} else {
				newX -= this->nCurOffset;
			}
		}
		pWidget->Move(newX, newY);

		pWidget->Paint(bUpdateRect);

		if (this->bIsVertical) {
			drawY += pWidget->GetH() + this->nPadding;
		} else {
			drawX += pWidget->GetW() + this->nPadding;
		}
	}
	if (this->bIsVertical) {
		if (this->nTotalSize > (int)this->h) {
			// Draw vertical scrollbar
		}
	} else {
		if (this->nTotalSize > (int)this->w) {
			// Draw horizontal scrollbar
		}
	}

	if (bParentIsNotStack)
		SDL_SetClipRect(pDestSurface, NULL);
}

//******************************************************************************
void CStackWidget::HandleMouseWheel(
//Handles a mouse wheel event.
//
//Params:
	const SDL_MouseWheelEvent &MouseWheelEvent)   //(in) Event to handle.
{
	if (MouseWheelEvent.y > 0 || MouseWheelEvent.x < 0)
	{
		this->nCurOffset-=5;
		if (this->nCurOffset < 0) this->nCurOffset = 0;
	}
	else if (MouseWheelEvent.y < 0 || MouseWheelEvent.x > 0)
	{
		this->nCurOffset+=5;
		if (this->bIsVertical) {
			if (this->nCurOffset + (int)this->h >= this->nTotalSize) this->nCurOffset = this->nTotalSize - this->h - 1;
		} else {
			if (this->nCurOffset + (int)this->w >= this->nTotalSize) this->nCurOffset = this->nTotalSize - this->w - 1;
		}
	}

	RequestPaint();
}

//*********************************************************************************************************
void CStackWidget::HandleAnimate()
{
}

//*********************************************************************************************************
void CStackWidget::AddWidget(CWidget* pWidget)
{
	ASSERT(pWidget);
	pWidget = CWidget::AddWidget(pWidget, true);

	// Add it to the bottom/right of the stack.  Resize the stack to fit the new element.
	if (this->bIsVertical) {
		if (pWidget->GetW() > this->w) this->w = pWidget->GetW();
		this->nTotalSize += pWidget->GetH();
		if (!this->widgets.empty())
			this->nTotalSize += this->nPadding;
		this->h  = (this->nTotalSize > this->nMaxSize && this->nMaxSize != 0) ? this->nMaxSize : this->nTotalSize;
	} else {
		if (pWidget->GetH() > this->h) this->h = pWidget->GetH();
		this->nTotalSize += pWidget->GetW();
		if (!this->widgets.empty())
			this->nTotalSize += this->nPadding;
		this->w  = (this->nTotalSize > this->nMaxSize && this->nMaxSize != 0) ? this->nMaxSize : this->nTotalSize;
	}
	this->widgets.push_back(pWidget);
}
