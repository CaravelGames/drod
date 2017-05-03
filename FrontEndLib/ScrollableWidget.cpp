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

#include "ScrollableWidget.h"

#include <BackEndLib/Assert.h>

//Coords within parts surface.
const int X_UP = 308;
const int Y_UP = 30;
const int X_UP_D = 308;
const int Y_UP_D = 54;
const int X_DOWN = 332;
const int Y_DOWN = 30;
const int X_DOWN_D = 332;
const int Y_DOWN_D = 54;
const int X_VPOS = 356;
const int Y_VPOS = 44;
const int X_VBACK = 356;
const int Y_VBACK = 0;

const int X_LEFT = 382;
const int Y_LEFT = 26;
const int X_LEFT_D = 382;
const int Y_LEFT_D = 50;
const int X_RIGHT = 406;
const int Y_RIGHT = 26;
const int X_RIGHT_D = 406;
const int Y_RIGHT_D = 50;
const int X_HPOS = 356;
const int Y_HPOS = 44;
const int X_HBACK = 382;
const int Y_HBACK = 0;

const UINT CY_TOP_BEVEL = 6;
const UINT CY_BOTTOM_BEVEL = 6;
const UINT CY_CENTER = 12;

const UINT CX_LEFT_BEVEL = 6;
const UINT CX_RIGHT_BEVEL = 6;
const UINT CX_CENTER = 12;

//Dimensions of areas drawn for widget.
const UINT CScrollableWidget::CX_VBACK = 24; //vertical scroll bar
const UINT CScrollableWidget::CY_VBACK = 44;
const UINT CScrollableWidget::CX_VPOS = 24;
const UINT CScrollableWidget::CY_VPOS = 24;
const UINT CScrollableWidget::CX_UP = 24;
const UINT CScrollableWidget::CY_UP = 24;
const UINT CScrollableWidget::CX_DOWN = 24;
const UINT CScrollableWidget::CY_DOWN = 24;

const UINT CScrollableWidget::CX_HBACK = 44;   //horizontal scroll bar
const UINT CScrollableWidget::CY_HBACK = 24;
const UINT CScrollableWidget::CX_HPOS = 24;
const UINT CScrollableWidget::CY_HPOS = 24;
const UINT CScrollableWidget::CX_LEFT = 24;
const UINT CScrollableWidget::CY_LEFT = 24;
const UINT CScrollableWidget::CX_RIGHT = 24;
const UINT CScrollableWidget::CY_RIGHT = 24;

//
//Public methods.
//

//******************************************************************************
CScrollableWidget::CScrollableWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH, //
	const WIDGETTYPE eSetType)
	: CFocusWidget(eSetType, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, eLastClickResult(SWCR_Nothing)
	, nPosClickX(0), nPosClickY(0)
	, nVPosClickY(0), nHPosClickX(0)
	, bIsUpButtonPressed(false), bIsDownButtonPressed(false)
	, bIsLeftButtonPressed(false), bIsRightButtonPressed(false)
{
	this->imageFilenames.push_back(string("Dialog"));

	CLEAR_RECT(this->ItemsRect);
	CLEAR_RECT(this->VPosRect);
	CLEAR_RECT(this->VScrollRect);
	CLEAR_RECT(this->UpRect);
	CLEAR_RECT(this->DownRect);
	CLEAR_RECT(this->HPosRect);
	CLEAR_RECT(this->HScrollRect);
	CLEAR_RECT(this->LeftRect);
	CLEAR_RECT(this->RightRect);
}

//***************************************************************************
CWidget* CScrollableWidget::AddWidget(
//Add a child widget to this widget.
//
//Params:
	CWidget *pWidget, //(in)   Widget to add.
	bool bLoad)       //(in)   If true widget will be loaded.  False is default.
//
//Returns:
//Pointer to new widget (same as in param) of NULL if a load failed.  Caller
//should not delete the widget after a successful or unsuccessful call.
{
	//Only a single child can be added.
	ASSERT(this->Children.empty());

	//Move and resize child to fill the container.
	//Make size allowances for displaying scroll bars if child widget's
	//desired size is about as large or larger than the container.
	SDL_Rect rect;
	GetRectAvailableForChildren(rect, pWidget->GetW(), pWidget->GetH());
	pWidget->Resize(rect.w, rect.h);
	CWidget *pRetWidget = CWidget::AddWidget(pWidget, bLoad);
	pWidget->Move(rect.x - this->x, rect.y - this->y);
	CalcAreas();   //update view
	return pRetWidget;
}

//******************************************************************************
void CScrollableWidget::CenterOn(const int wX, const int wY)
//Center view of contained widget to specified position, as close as possible.
{
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);

	//Center on point.
	ScrollAbsolute(this->ItemsRect.w/2 - wX, this->ItemsRect.h/2 - wY);

	//Do not allow view to scroll too far.
	if (this->nChildrenScrollOffsetX > 0)
		this->nChildrenScrollOffsetX = 0;
	else
	{
		const int xGap = (Sint16)this->ItemsRect.w - (this->nChildrenScrollOffsetX +
				(Sint16)childrenRect.w);
		if (xGap > 0)
			this->nChildrenScrollOffsetX += xGap;
	}
	if (this->nChildrenScrollOffsetY > 0)
		this->nChildrenScrollOffsetY = 0;
	else
	{
		const int yGap = (Sint16)this->ItemsRect.h - (this->nChildrenScrollOffsetY +
				(Sint16)childrenRect.h);
		if (yGap > 0)
			this->nChildrenScrollOffsetY += yGap;
	}

	CalcAreas();
	if (IsVisible())
		RequestPaint();
}

//*****************************************************************************
void CScrollableWidget::GetRectAvailableForChildren(
//OUT: rect relative to this widget that children can be placed in
//
//Params:
	SDL_Rect &rect,   //(out)
	const UINT wDesiredW, const UINT wDesiredH)   //(in) [default = (0,0)]
const
{
	//Children shouldn't be placed on top of the inset bevel.
	rect.x = this->x + CX_INSET_BORDER;
	rect.y = this->y + CY_INSET_BORDER;
	rect.w = this->w - CX_INSET_BORDER*2;
	rect.h = this->h - CY_INSET_BORDER*2;

	//If scroll bars will be required to show the child widget,
	//save room for them.
	bool bVerticalScrollBar = false;
	if (wDesiredH > (UINT)rect.h)
	{
		rect.w -= CX_UP;
		bVerticalScrollBar = true;
	}
	if (wDesiredW > (UINT)rect.w)
	{
		rect.h -= CY_LEFT;
		if (wDesiredH > (UINT)rect.h && !bVerticalScrollBar)
			rect.w -= CX_UP;
	}
}

//*****************************************************************************
void CScrollableWidget::Move(
//Move widget, and recalc clicking areas.
//
//Params:
	const int nSetX, const int nSetY)   //(in)
{
	CWidget::Move(nSetX, nSetY);

	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	//Draw inset area where text appears.
	SDL_Surface *pDestSurface = GetDestSurface();
	DrawInset(this->x, this->y, this->w, this->h, this->images[0], 
			pDestSurface, this->Children.empty(), bUpdateRect);

	//Draw scroll bars if needed.
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const bool bDrawVertScrollBar = (childrenRect.h > this->ItemsRect.h);
	const bool bDrawHorizScrollBar = (childrenRect.w > this->ItemsRect.w);
	if (bDrawVertScrollBar)
	{
		DrawVertScrollBar(pDestSurface);
		if (bUpdateRect)
		{
			UpdateRect(this->VScrollRect);
			UpdateRect(this->UpRect);
			UpdateRect(this->DownRect);
		}
	}
	if (bDrawHorizScrollBar)
	{
		DrawHorizScrollBar(pDestSurface);
		if (bUpdateRect)
		{
			UpdateRect(this->HScrollRect);
			UpdateRect(this->LeftRect);
			UpdateRect(this->RightRect);
		}
	}

	PaintChildren(bUpdateRect);
}

//
//Protected methods.
//

//*****************************************************************************
void CScrollableWidget::CalcAreas()
//Calculate coords and dimensions of areas within widget.
{
	ASSERT(this->w > GetRequiredComponentWidth());
	ASSERT(this->h > GetRequiredComponentHeight());

	//Figure out if I need to draw a horizontal/vertical scroll bar.
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	GetRectAvailableForChildren(this->ItemsRect, childrenRect.w, childrenRect.h);
	const bool bDrawVertScrollBar = (childrenRect.h > this->ItemsRect.h);
	const bool bDrawHorizScrollBar = (childrenRect.w > this->ItemsRect.w);

	//If scroll bar is missing, then contained widget should not be scrolled
	//in that direction.
	if (!bDrawVertScrollBar)
		ScrollAbsolute(this->nChildrenScrollOffsetX, 0);
	if (!bDrawHorizScrollBar)
		ScrollAbsolute(0, this->nChildrenScrollOffsetY);

	//Calc vertical scroll bar if needed.
	if (!bDrawVertScrollBar)
	{
		CLEAR_RECT(this->VScrollRect);
		CLEAR_RECT(this->UpRect);
		CLEAR_RECT(this->DownRect);
	}
	else
	{
		//Calc up button.
		const int nX = this->x + this->w - CX_UP - CX_INSET_BORDER;
		SET_RECT(this->UpRect, nX, this->y + CY_INSET_BORDER, CX_UP, CY_UP);

		//Calc down button.
		SET_RECT(this->DownRect, nX, 
				this->y + this->h - CY_INSET_BORDER - CY_DOWN, CX_DOWN, CY_DOWN);

		//Calc scroll bar background between the two buttons.
		this->VScrollRect.x = this->DownRect.x;
		this->VScrollRect.y = this->UpRect.y + this->UpRect.h;
		this->VScrollRect.w = CX_VBACK;
		this->VScrollRect.h = this->DownRect.y - this->VScrollRect.y;

		//Will position button fit in space between up and down buttons?
		if (this->VScrollRect.h < (int)CY_VPOS)  //No.
			CLEAR_RECT(this->VPosRect);
		else                    //Yes.
		{
			//Figure percentage position button is from top of contained widget.
			ASSERT(childrenRect.h - this->ItemsRect.h > 0);
			ASSERT(this->nChildrenScrollOffsetY <= 0);
			const double dblVPosPercent = (double) -this->nChildrenScrollOffsetY /
					(double) (childrenRect.h - this->ItemsRect.h);
			ASSERT(dblVPosPercent >= 0.0);
			ASSERT(dblVPosPercent <= 1.0);

			//Calc position button.
			this->VPosRect.h = (Uint16)(this->VScrollRect.h * ((double) this->ItemsRect.h /
					(double) childrenRect.h));
			if (this->VPosRect.h < (int)CY_VPOS) this->VPosRect.h = CY_VPOS;
			this->VPosRect.w = CX_VPOS;
			this->VPosRect.x = this->VScrollRect.x;
			this->VPosRect.y = this->VScrollRect.y + 
					(Sint16)(dblVPosPercent * (this->VScrollRect.h - this->VPosRect.h));
		}
	}

	//Calc horizontal scroll bar if needed.
	if (!bDrawHorizScrollBar)
	{
		CLEAR_RECT(this->HScrollRect);
		CLEAR_RECT(this->LeftRect);
		CLEAR_RECT(this->RightRect);
	}
	else
	{
		//Calc left button.
		const int nY = this->y + this->h - CY_LEFT - CY_INSET_BORDER;
		SET_RECT(this->LeftRect, this->x + CX_INSET_BORDER, nY, CX_LEFT, CY_LEFT);

		//Calc right button.
		SET_RECT(this->RightRect, this->x + this->w - CX_INSET_BORDER - CX_RIGHT,
				nY, CX_RIGHT, CY_RIGHT);
		if (bDrawVertScrollBar)
		{
			//Don't overlap vertical scroll bar.
			this->RightRect.x -= this->UpRect.w;   
		}

		//Calc scroll bar background between the two buttons.
		this->HScrollRect.x = this->LeftRect.x + this->LeftRect.w;
		this->HScrollRect.y = this->LeftRect.y;
		this->HScrollRect.w = this->RightRect.x - this->HScrollRect.x;
		this->HScrollRect.h = CY_HBACK;

		//Will position button fit in space between left and right buttons?
		if (this->HScrollRect.w < (int)CX_HPOS)  //No.
			CLEAR_RECT(this->HPosRect);
		else                    //Yes.
		{
			//Figure percentage position button is from left of contained widget.
			ASSERT(childrenRect.w - this->ItemsRect.w > 0);
			ASSERT(this->nChildrenScrollOffsetX <= 0);
			const double dblHPosPercent = (double) -this->nChildrenScrollOffsetX /
					(double) (childrenRect.w - this->ItemsRect.w);
			ASSERT(dblHPosPercent >= 0.0);
			ASSERT(dblHPosPercent <= 1.0);

			//Calc position button.
			this->HPosRect.w = (Uint16)(this->HScrollRect.w * ((double) this->ItemsRect.w /
					(double) childrenRect.w));
			if (this->HPosRect.w < (int)CX_HPOS) this->HPosRect.w = CX_HPOS;
			this->HPosRect.h = CY_HPOS;
			this->HPosRect.x = this->HScrollRect.x + 
					(Sint16)(dblHPosPercent * (this->HScrollRect.w - this->HPosRect.w));
			this->HPosRect.y = this->HScrollRect.y;
		}
	}

	//When child widget's size has been set to (0,0), resize it to fit the area.
	if (this->Children.size() > 0)
	{
		CWidget *pWidget = this->Children.front();
		if (pWidget->GetW() == 0 && pWidget->GetH() == 0)
			pWidget->Resize(this->ItemsRect.w, this->ItemsRect.h);
	}
}

//*****************************************************************************
void CScrollableWidget::CalcAreas_VerticalOnly(
//Calculate coords and dimensions of areas for vertical scrolling only.
//
//Params:
	const UINT wContentH, //height of the data filling the window
	const UINT wDisplayH, //height of the view window
	UINT& wViewTopY) //(in/out) relative Y of part of content shown at the top of the view window
{
	ASSERT(this->w > GetRequiredComponentWidth());
	ASSERT(this->h > GetRequiredComponentHeight());

	//Figure out whether a scroll bar should be drawn.
	const bool bDrawScrollBar = wContentH > wDisplayH;
	if (!bDrawScrollBar)
		wViewTopY = 0;

	if (!bDrawScrollBar)
	{
		//Scroll bar not needed.
		CLEAR_RECT(this->VScrollRect);
		CLEAR_RECT(this->UpRect);
		CLEAR_RECT(this->DownRect);
	} else {
		//Calc scroll bar.

		//Calc up button.
		SET_RECT(this->UpRect, this->x + this->w - CX_UP - CX_INSET_BORDER,
				this->y + CY_INSET_BORDER, CX_UP, CY_UP);

		//Calc down button.
		SET_RECT(this->DownRect, this->x + this->w - CX_UP - CX_INSET_BORDER,
				this->y + this->h - CY_INSET_BORDER - CY_DOWN, CX_UP, CY_UP);

		//Calc scroll bar background between the two buttons.
		this->VScrollRect.x = this->DownRect.x;
		this->VScrollRect.y = this->y + CY_INSET_BORDER + CY_UP;
		this->VScrollRect.w = CX_VBACK;
		this->VScrollRect.h = this->DownRect.y - this->y - CY_INSET_BORDER - CY_UP;

		//Will position button fit in space between up and down buttons?
		if (this->VScrollRect.h < (int)CY_VPOS)  //No.
			CLEAR_RECT(this->VPosRect);
		else                    //Yes.
		{
			//Figure percentage thumb button is from the top of the scroll bar.
			//Don't allow the view to go past the bottom of the content.
			if (wViewTopY + wDisplayH > wContentH)
				wViewTopY = wContentH - wDisplayH;
			const double dblPosPercent = double(wViewTopY) /
					double(wContentH - wDisplayH);
			ASSERT(dblPosPercent <= 1.0);

			//Calc position button.
			this->VPosRect.h = Uint16(this->VScrollRect.h * (double(wDisplayH) /
					double(wContentH)));
			if (this->VPosRect.h < (int)CY_VPOS) this->VPosRect.h = CY_VPOS;
			this->VPosRect.w = CX_VPOS;
			this->VPosRect.x = this->VScrollRect.x;
			this->VPosRect.y = this->y + CY_INSET_BORDER + CY_UP +
					(Sint16)(dblPosPercent * (this->VScrollRect.h - this->VPosRect.h));
		}
	}

	//If no content is present, then items rect is inactive.
	if (!wContentH)
	{
		CLEAR_RECT(this->ItemsRect);
	} else {
		//Calc item rect.
		this->ItemsRect.x = this->x + CX_INSET_BORDER;
		this->ItemsRect.y = this->y + CY_INSET_BORDER;
		this->ItemsRect.w = this->w - (CX_INSET_BORDER * 2) - (bDrawScrollBar ?
				CX_UP : 0);
		this->ItemsRect.h = wDisplayH;
	}
}

//******************************************************************************
void CScrollableWidget::ChildResized()
{
	ScrollAbsolute(0, 0);
	CalcAreas();
	RequestPaint(false);
}

//******************************************************************************
CScrollableWidget::SW_CLICK_RESULT CScrollableWidget::ClickAtCoords(
//Updates list box in response to a mouse click at specified coords.
//
//Params:
	const int nX, const int nY)   //(in) Click coords.
//
//Returns:
//An SWCR_* constant indicating what happened.
{
	//Check for click inside view area.
	if (IS_IN_RECT(nX, nY, this->ItemsRect))
	{
		return SWCR_Selection;
	}

	//Vertical scroll bar.
	if (this->VPosRect.w > 0)
	{
		//Check for click on vertical position button.
		if (IS_IN_RECT(nX, nY, this->VPosRect))
		{
			this->nPosClickY = nY;
			this->nVPosClickY = this->VPosRect.y;
			return SWCR_VPosButton;
		}

		//Check for click inside vertical scroll area, but not on vertical
		//position button since we just checked that.
		if (IS_IN_RECT(nX, nY, this->VScrollRect))
		{
			if (nY < this->VPosRect.y)
			{
				ScrollUpOnePage();
				return SWCR_PageUp;
			}
			ScrollDownOnePage();
			return SWCR_PageDown;
		}

		//Check for click inside scroll bar up button.
		if (IS_IN_RECT(nX, nY, this->UpRect))
		{
			ScrollUpOneLine();
			return SWCR_UpButton;
		}

		//Check for click inside scroll bar down button.
		if (IS_IN_RECT(nX, nY, this->DownRect))
		{
			ScrollDownOneLine();
			return SWCR_DownButton;
		}
	}

	//Horizontal scroll bar.
	if (this->HPosRect.h > 0)
	{
		//Check for click on horizontal position button.
		if (IS_IN_RECT(nX, nY, this->HPosRect))
		{
			this->nPosClickX = nX;
			this->nHPosClickX = this->HPosRect.x;
			return SWCR_HPosButton;
		}

		//Check for click inside horizontal scroll area, but not on horizontal
		//position button since we just checked that.
		if (IS_IN_RECT(nX, nY, this->HScrollRect))
		{
			if (nX < this->HPosRect.x)
			{
				ScrollLeftOnePage();
				return SWCR_PageLeft;
			}
			ScrollRightOnePage();
			return SWCR_PageRight;
		}

		//Check for click inside scroll bar left button.
		if (IS_IN_RECT(nX, nY, this->LeftRect))
		{
			ScrollLeftOneLine();
			return SWCR_LeftButton;
		}

		//Check for click inside scroll bar right button.
		if (IS_IN_RECT(nX, nY, this->RightRect))
		{
			ScrollRightOneLine();
			return SWCR_RightButton;
		}
	}

	//Click did not cause an update of the widget.
	return SWCR_Nothing;
}

//******************************************************************************
void CScrollableWidget::DragHorizPosButton(
//Updates horizontal position button and contained widget position based on X coordinate
//from mouse dragging after a click on the position button.
//
//Param:
	const int nX)  //(in)   Horizontal mouse coord.
{
	//Shouldn't be showing the horizontal scroll bar if all the children fit.
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	ASSERT(childrenRect.w > this->ItemsRect.w);

	//Figure difference from click coord to current coord.
	const int nMoveX = nX - this->nPosClickX;
	//Move the view to new location.
	if (this->nHPosClickX + nMoveX <= this->HScrollRect.x)
		ScrollAbsolute(0, this->nChildrenScrollOffsetY);
	else if (this->nHPosClickX + this->HPosRect.w + nMoveX >= this->RightRect.x)
		ScrollAbsolute(-(childrenRect.w - this->ItemsRect.w), this->nChildrenScrollOffsetY);
	else
	{
		//Calculate new relative position of slider.
		const float dHPosPercent = ((this->nHPosClickX + nMoveX) - this->HScrollRect.x) /
				(float) (this->HScrollRect.w - this->HPosRect.w);
		ASSERT(dHPosPercent > 0.0 && dHPosPercent < 1.0);
		int dItemsHPos = -(int)(((float)(childrenRect.w - this->ItemsRect.w)
				* dHPosPercent) + 0.5); //round
		if (dItemsHPos == this->nChildrenScrollOffsetX)
			dItemsHPos -= dHPosPercent > 0.0 ? 1 : -1;
		ScrollAbsolute(dItemsHPos, this->nChildrenScrollOffsetY);
	}

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::DragVertPosButton(
//Updates vertical position button and contained widget position based on Y coordinate
//from mouse dragging after a click on the position button.
//
//Param:
	const int nY)  //(in)   Vertical mouse coord.
{
	//Shouldn't be showing the vertical scroll bar if all the children fit.
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	ASSERT(childrenRect.h > this->ItemsRect.h);

	//Figure difference from click coord to current coord.
	const int nMoveY = nY - this->nPosClickY;
	//Move the view to new location.
	if (this->nVPosClickY + nMoveY <= this->VScrollRect.y)
		ScrollAbsolute(this->nChildrenScrollOffsetX, 0);
	else if (this->nVPosClickY + this->VPosRect.h + nMoveY >= this->DownRect.y)
		ScrollAbsolute(this->nChildrenScrollOffsetX, -(childrenRect.h - this->ItemsRect.h));
	else
	{
		//Calculate new relative position of slider.
		const float dVPosPercent = ((this->nVPosClickY + nMoveY) - this->VScrollRect.y) /
				(float) (this->VScrollRect.h - this->VPosRect.h);
		ASSERT(dVPosPercent > 0.0 && dVPosPercent < 1.0);
		int dItemsVPos = -(int)(((float)(childrenRect.h - this->ItemsRect.h)
				* dVPosPercent) + 0.5); //round
		if (dItemsVPos == this->nChildrenScrollOffsetY)
			dItemsVPos -= dVPosPercent > 0.0 ? 1 : -1;
		ScrollAbsolute(this->nChildrenScrollOffsetX, dItemsVPos);
	}

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::DrawVertScrollBar(SDL_Surface *pDestSurface)
{
	//Draw up button.
	SDL_Rect src = MAKE_SDL_RECT((this->bIsUpButtonPressed) ? X_UP_D : X_UP,
		(this->bIsUpButtonPressed) ? Y_UP_D : Y_UP, CX_UP, CY_UP);
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &this->UpRect);

	//Draw down button.
	src.x = (this->bIsDownButtonPressed) ? X_DOWN_D : X_DOWN;
	src.y = (this->bIsDownButtonPressed) ? Y_DOWN_D : Y_DOWN;
	src.w = CX_DOWN;
	src.h = CY_DOWN;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &this->DownRect);

	//Draw scroll bar background between the two buttons.
	SDL_Rect dest = {this->VScrollRect.x, this->VScrollRect.y, CX_VBACK, CY_VBACK};
	src.x = X_VBACK;
	src.y = Y_VBACK;
	src.w = CX_VBACK;
	src.h = dest.h = CY_VBACK;
	for (; dest.y < this->DownRect.y; dest.y += CY_VBACK)
	{
		if (dest.y + CY_VBACK > static_cast<UINT>(this->DownRect.y))
			dest.h = src.h = this->DownRect.y - dest.y; //Clip the blit to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw position button if it will fit in space between up and down buttons.
	if (this->VScrollRect.h >= (int)CY_VPOS)
	{
		src.x = X_VPOS;
		src.y = Y_VPOS;
		src.w = CX_VPOS;

		//Draw position button.
		if (this->VPosRect.h == CY_VPOS) //Draw position button at its smallest height.
		{
			src.h = CY_VPOS;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &(this->VPosRect));
		}
		else  //Draw stretched position button.
		{
			//Draw top bevel.
			dest.x = this->VPosRect.x;
			dest.y = this->VPosRect.y;
			dest.w = this->VPosRect.w;
			src.h = dest.h = CY_TOP_BEVEL;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

			//Draw stretched center of position button.
			src.y += CY_TOP_BEVEL;
			src.h = dest.h = CY_CENTER;
			const int yBottomBevel = this->VPosRect.y + this->VPosRect.h - CY_BOTTOM_BEVEL;
			for (dest.y += CY_TOP_BEVEL; dest.y < yBottomBevel; dest.y += CY_CENTER)
			{
				if (dest.y + CY_CENTER >= (UINT) yBottomBevel)
					//Clip the blit to remaining height.
					dest.h = src.h = yBottomBevel - dest.y;
				SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
			}

			//Draw bottom bevel.
			src.h = dest.h = CY_BOTTOM_BEVEL;
			src.y += CY_CENTER;
			dest.y = yBottomBevel;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
		}
	}
}

//******************************************************************************
void CScrollableWidget::DrawHorizScrollBar(SDL_Surface *pDestSurface)
{
	//Draw left button.
	SDL_Rect src = MAKE_SDL_RECT((this->bIsLeftButtonPressed) ? X_LEFT_D : X_LEFT,
		(this->bIsLeftButtonPressed) ? Y_LEFT_D : Y_LEFT, CX_LEFT, CY_LEFT);
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &this->LeftRect);

	//Draw right button.
	src.x = (this->bIsRightButtonPressed) ? X_RIGHT_D : X_RIGHT;
	src.y = (this->bIsRightButtonPressed) ? Y_RIGHT_D : Y_RIGHT;
	src.w = CX_RIGHT;
	src.h = CY_RIGHT;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &this->RightRect);

	//Draw scroll bar background between the two buttons.
	SDL_Rect dest = {this->HScrollRect.x, this->HScrollRect.y, CX_HBACK, CY_HBACK};
	src.x = X_HBACK;
	src.y = Y_HBACK;
	src.w = dest.w = CX_HBACK;
	src.h = CY_HBACK;
	for (; dest.x < this->RightRect.x; dest.x += CX_HBACK)
	{
		if (dest.x + CX_HBACK > static_cast<UINT>(this->RightRect.x))
			dest.w = src.w = this->RightRect.x - dest.x; //Clip the blit to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw position button if it will fit in space between left and right buttons.
	if (this->HScrollRect.w >= (int)CX_HPOS)
	{
		src.x = X_HPOS;
		src.y = Y_HPOS;
		src.h = CY_HPOS;

		//Draw position button.
		if (this->HPosRect.w == CX_HPOS) //Draw position button at its smallest height.
		{
			src.h = CX_HPOS;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &(this->HPosRect));
		}
		else  //Draw stretched position button.
		{
			//Draw left bevel.
			dest.x = this->HPosRect.x;
			dest.y = this->HPosRect.y;
			src.w = dest.w = CX_LEFT_BEVEL;
			dest.h = this->HPosRect.h;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

			//Draw stretched center of position button.
			src.x += CX_LEFT_BEVEL;
			src.w = dest.w = CX_CENTER;
			const int xRightBevel = this->HPosRect.x + this->HPosRect.w - CX_RIGHT_BEVEL;
			for (dest.x += CX_LEFT_BEVEL; dest.x < xRightBevel; dest.x += CX_CENTER)
			{
				if (dest.x + CX_CENTER >= (UINT) xRightBevel)
					//Clip the blit to remaining height.
					dest.w = src.w = xRightBevel - dest.x;
				SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
			}

			//Draw right bevel.
			src.w = dest.w = CX_RIGHT_BEVEL;
			src.x += CX_CENTER;
			dest.x = xRightBevel;
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
		}
	}
}

//*****************************************************************************
void CScrollableWidget::GetChildClippingRect(
//Output the area that this widget wants its children to be drawn within.
//
//Params:
	SDL_Rect &ChildClipRect)   //(out) the area this widget wants its children clipped to
const
{
	//Clip to the child view area.
	ChildClipRect = this->ItemsRect;
}

//******************************************************************************
void CScrollableWidget::HandleDrag(
//Handles a mouse motion event while the button is down.
//
//Params:
	const SDL_MouseMotionEvent &Motion)
{
	if (this->eLastClickResult == SWCR_VPosButton)
	{
		if (Motion.yrel != 0)
		{
			DragVertPosButton(Motion.y);
			RequestPaint();
		}
	}

	if (this->eLastClickResult == SWCR_HPosButton)
	{
		if (Motion.xrel != 0)
		{
			DragHorizPosButton(Motion.x);
			RequestPaint();
		}
	}
}

//******************************************************************************
void CScrollableWidget::HandleMouseDown(
//Handles a mouse down event.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	if (this->eLastClickResult != SWCR_Nothing)
	{
		//Can happen if a selection change was made, causing a dialog box to
		//pop up before the mouse up event is received.
		this->bIsDownButtonPressed = this->bIsUpButtonPressed = false;
		this->bIsLeftButtonPressed = this->bIsRightButtonPressed = false;
	}

	this->eLastClickResult = ClickAtCoords(MouseButtonEvent.x, MouseButtonEvent.y);
	switch (this->eLastClickResult)
	{
		case SWCR_Nothing:
		return; //No changes to paint.

		case SWCR_UpButton:
			this->bIsUpButtonPressed = true;
		break;

		case SWCR_DownButton:
			this->bIsDownButtonPressed = true;
		break;

		case SWCR_LeftButton:
			this->bIsLeftButtonPressed = true;
		break;

		case SWCR_RightButton:
			this->bIsRightButtonPressed = true;
		break;

		default:
		break;
	}
	RequestPaint();
}

//******************************************************************************
void CScrollableWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	switch (KeyboardEvent.keysym.sym)
	{
		case SDLK_UP: case SDLK_KP_8:
			ScrollUpOneLine();
		break;

		case SDLK_DOWN: case SDLK_KP_2:
			ScrollDownOneLine();
		break;

		case SDLK_LEFT: case SDLK_KP_4:
			ScrollLeftOneLine();
		break;

		case SDLK_RIGHT: case SDLK_KP_6:
			ScrollRightOneLine();
		break;

		case SDLK_HOME: case SDLK_KP_7:
			ScrollLeftOnePage();
		break;

		case SDLK_END: case SDLK_KP_1:
			ScrollRightOnePage();
		break;

		case SDLK_PAGEUP: case SDLK_KP_9:
			ScrollUpOnePage();
		break;

		case SDLK_PAGEDOWN: case SDLK_KP_3:
			ScrollDownOnePage();
		break;

		default:
		return;
	}

	RequestPaint();
}

//******************************************************************************
void CScrollableWidget::HandleMouseDownRepeat(
//Handles a mouse down repeat event.  This is caused when the user holds the mouse
//button down for a period of time.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in) Mouse down event to handle.
{
	//Just repeat the results of last click result if coords are still in
	//correct area.
	switch (this->eLastClickResult)
	{
		case SWCR_UpButton:
			ASSERT(this->bIsUpButtonPressed);
			if (IS_IN_RECT(Button.x, Button.y, this->UpRect))
				ScrollUpOneLine();
		break;

		case SWCR_DownButton:
			ASSERT(this->bIsDownButtonPressed);
			if (IS_IN_RECT(Button.x, Button.y, this->DownRect))
				ScrollDownOneLine();
		break;

		case SWCR_PageUp:
		case SWCR_PageDown:
			if (IS_IN_RECT(Button.x, Button.y, this->VScrollRect) &&
					!IS_IN_RECT(Button.x, Button.y, this->VPosRect))
			{
				if (Button.y < this->VPosRect.y)
					ScrollUpOnePage();
				else
					ScrollDownOnePage();
			}
		break;
		
		case SWCR_LeftButton:
			ASSERT(this->bIsLeftButtonPressed);
			if (IS_IN_RECT(Button.x, Button.y, this->LeftRect))
				ScrollLeftOneLine();
		break;

		case SWCR_RightButton:
			ASSERT(this->bIsRightButtonPressed);
			if (IS_IN_RECT(Button.x, Button.y, this->RightRect))
				ScrollRightOneLine();
		break;

		case SWCR_PageLeft:
		case SWCR_PageRight:
			if (IS_IN_RECT(Button.x, Button.y, this->HScrollRect) &&
					!IS_IN_RECT(Button.x, Button.y, this->HPosRect))
			{
				if (Button.x < this->HPosRect.x)
					ScrollLeftOnePage();
				else
					ScrollRightOnePage();
			}
		break;
		
		default://Nothing happened.
		return; 
	}
	RequestPaint();
}

//******************************************************************************
void CScrollableWidget::HandleMouseUp(
//Handles a mouse up event.
//
//Params:
	const SDL_MouseButtonEvent &/*Button*/)   //(in) Event to handle.
{
	ASSERT(!(this->bIsUpButtonPressed && this->bIsDownButtonPressed));

	//Unpress anything that is pressed.
	if (this->bIsUpButtonPressed)
	{
		ASSERT(this->eLastClickResult == SWCR_UpButton);
		this->bIsUpButtonPressed = false;
	}
	else if (this->bIsDownButtonPressed)
	{
		ASSERT(this->eLastClickResult == SWCR_DownButton);
		this->bIsDownButtonPressed = false;
	}
	else if (this->bIsLeftButtonPressed)
	{
		ASSERT(this->eLastClickResult == SWCR_LeftButton);
		this->bIsLeftButtonPressed = false;
	}
	else if (this->bIsRightButtonPressed)
	{
		ASSERT(this->eLastClickResult == SWCR_RightButton);
		this->bIsRightButtonPressed = false;
	}
	RequestPaint();

	//Reset click result.
	this->eLastClickResult = SWCR_Nothing;
}

//******************************************************************************
void CScrollableWidget::HandleMouseWheel(
//Handles a mouse wheel event.
//
//Params:
	const SDL_MouseWheelEvent &MouseWheelEvent)   //(in) Event to handle.
{
	const bool bVerticalScrollBar = this->VScrollRect.w > 0;
	if (MouseWheelEvent.y > 0 || MouseWheelEvent.x < 0)
	{
		if (bVerticalScrollBar)
			ScrollUpOneLine();
		else
			ScrollLeftOneLine();
	}
	else if (MouseWheelEvent.y < 0 || MouseWheelEvent.x > 0)
	{
		if (bVerticalScrollBar)
			ScrollDownOneLine();
		else
			ScrollRightOneLine();
	}

	RequestPaint();
}

//******************************************************************************
void CScrollableWidget::ScrollDownPixels(const UINT wPixels)
//Scroll view down n pixels (i.e. move viewed widgets up).
{
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightBelowView = (childrenRect.y + childrenRect.h) - (this->ItemsRect.y + this->ItemsRect.h);

	if (nHeightBelowView <= 0) return;
	if (nHeightBelowView >= (int)wPixels)
		Scroll(0, -(int)wPixels);
	else
		Scroll(0, -nHeightBelowView);

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollDownOneLine(const UINT wLines)   //[default=1]
//Scroll view down one "line" (i.e. move viewed widgets up).
{
	static const UINT wPixelScrollAmount = 15;  //pixel scroll increment for a "line"
	ScrollDownPixels(wPixelScrollAmount * wLines);
}

//******************************************************************************
void CScrollableWidget::ScrollDownOnePage()
//Scroll view down one page (i.e. move viewed widgets up).
{
	ScrollDownPixels(this->ItemsRect.h);
}

//******************************************************************************
void CScrollableWidget::ScrollUpOneLine(const UINT wLines)   //[default=1]
//Scroll view up one "line" (i.e. move viewed widgets down).
{
	static const UINT wPixelScrollAmount = 15;  //pixel scroll increment for a "line"
	const UINT wScrollAmount = wPixelScrollAmount * wLines;

	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightAboveView = this->ItemsRect.y - childrenRect.y;

	if (nHeightAboveView <= 0) return;
	if (nHeightAboveView >= static_cast<int>(wScrollAmount))
		Scroll(0, wScrollAmount);
	else
		Scroll(0, nHeightAboveView);

	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollUpOnePage()
//Scroll view up one page (i.e. move viewed widgets down).
{
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightAboveView = this->ItemsRect.y - childrenRect.y;

	if (nHeightAboveView <= 0) return;
	if (nHeightAboveView >= this->ItemsRect.h)
		Scroll(0, this->ItemsRect.h);
	else
		Scroll(0, nHeightAboveView);

	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollLeftOneLine(const UINT wLines)   //[default=1]
//Scroll view left one "line" (i.e. move viewed widgets right).
{
	static const UINT wPixelScrollAmount = 15;  //pixel scroll increment for a "line"
	const UINT wScrollAmount = wPixelScrollAmount * wLines;

	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightToLeftOfView = this->ItemsRect.x - childrenRect.x;

	if (nHeightToLeftOfView <= 0) return;
	if (nHeightToLeftOfView >= static_cast<int>(wScrollAmount))
		Scroll(wScrollAmount, 0);
	else
		Scroll(nHeightToLeftOfView, 0);

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollLeftOnePage()
//Scroll view left one page (i.e. move viewed widgets right).
{
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightToLeftOfView = this->ItemsRect.x - childrenRect.x;

	if (nHeightToLeftOfView <= 0) return;
	if (nHeightToLeftOfView >= this->ItemsRect.w)
		Scroll(this->ItemsRect.w, 0);
	else
		Scroll(nHeightToLeftOfView, 0);

	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollRightOneLine(const UINT wLines)   //[default=1]
//Scroll view right one "line" (i.e. move viewed widgets left).
{
	static const UINT wPixelScrollAmount = 15;  //pixel scroll increment for a "line"
	const int nScrollAmount = wPixelScrollAmount * wLines;

	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightToRightOfView = (childrenRect.x + childrenRect.w) - (this->ItemsRect.x + this->ItemsRect.w);

	if (nHeightToRightOfView <= 0) return;
	if (nHeightToRightOfView >= nScrollAmount)
		Scroll(-nScrollAmount, 0);
	else
		Scroll(-nHeightToRightOfView, 0);

	CalcAreas();
}

//******************************************************************************
void CScrollableWidget::ScrollRightOnePage()
//Scroll view right one page (i.e. move viewed widgets left).
{
	SDL_Rect childrenRect;
	GetRectContainingChildren(childrenRect);
	const int nHeightToRightOfView = (childrenRect.x + childrenRect.w) - (this->ItemsRect.x + this->ItemsRect.w);

	if (nHeightToRightOfView <= 0) return;
	if (nHeightToRightOfView >= this->ItemsRect.w)
		Scroll(-this->ItemsRect.h, 0);
	else
		Scroll(-nHeightToRightOfView, 0);

	CalcAreas();
}
