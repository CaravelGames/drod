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

#ifndef SCROLLABLEWIDGET_H
#define SCROLLABLEWIDGET_H

#include "FocusWidget.h"
#include "Inset.h"

//******************************************************************************
class CScrollableWidget : public CFocusWidget
{
public:
	CScrollableWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const WIDGETTYPE eSetType=WT_Scrollable);

	virtual CWidget *    AddWidget(CWidget *pNewWidget, bool bLoad = false);
	void           CenterOn(const int wX, const int wY);
	virtual void   GetChildClippingRect(SDL_Rect &ChildClipRect) const;
	void           GetRectAvailableForChildren(SDL_Rect &rect,
			const UINT wDesiredW=0, const UINT wDesiredH=0) const;
	virtual void   Move(const int nSetX, const int nSetY);
	virtual void   Paint(bool bUpdateRect = true);
	virtual bool   ParentMustPaintChildren() const {return true;}

	enum SW_CLICK_RESULT
	{
		SWCR_Nothing = 0,
		SWCR_UpButton,    //vertical scroll bar
		SWCR_DownButton,
		SWCR_VPosButton,
		SWCR_PageUp,
		SWCR_PageDown,
		SWCR_LeftButton,  //horizontal scroll bar
		SWCR_RightButton,
		SWCR_HPosButton,
		SWCR_PageLeft,
		SWCR_PageRight,
		SWCR_Selection,   //click in main area
		LBCR_NewSelection //new selection in main area
	};

	UINT           GetVScrollBarWidth() const {return CX_UP;}
	UINT           GetHScrollBarHeight() const {return CY_LEFT;}
	virtual void   ScrollDownPixels(const UINT wPixels);
	virtual void   ScrollDownOneLine(const UINT wLines=1);
	virtual void   ScrollDownOnePage();
	virtual void   ScrollUpOneLine(const UINT wLines=1);
	virtual void   ScrollUpOnePage();
	virtual void   ScrollLeftOneLine(const UINT wLines=1);
	virtual void   ScrollLeftOnePage();
	virtual void   ScrollRightOneLine(const UINT wLines=1);
	virtual void   ScrollRightOnePage();
	
	SDL_Rect GetItemsRect() const {return this->ItemsRect;}

	static UINT GetRequiredComponentWidth() { return CX_UP + CX_INSET_BORDER * 2; }
	static UINT GetRequiredComponentHeight() { return CY_UP + CY_DOWN + CY_INSET_BORDER * 2; }

protected:
	virtual void   CalcAreas();
	void           CalcAreas_VerticalOnly(const UINT wContentH, const UINT wDisplayH,
			UINT& wViewTopY);
	virtual void   ChildResized();
	virtual SW_CLICK_RESULT ClickAtCoords(const int nX, const int nY);
	void           DrawHorizScrollBar(SDL_Surface *pDestSurface);
	void           DrawVertScrollBar(SDL_Surface *pDestSurface);

	virtual void   HandleDrag(const SDL_MouseMotionEvent &MouseMotionEvent);
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseDownRepeat(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);
	virtual void   HandleMouseWheel(const SDL_MouseWheelEvent &Wheel);

	virtual void   DragHorizPosButton(const int nY);
	virtual void   DragVertPosButton(const int nY);

	SDL_Rect       ItemsRect;
	SDL_Rect       VPosRect;   //vertical scroll regions
	SDL_Rect       VScrollRect;
	SDL_Rect       UpRect;
	SDL_Rect       DownRect;
	SDL_Rect       HPosRect;   //horizontal scroll regions
	SDL_Rect       HScrollRect;
	SDL_Rect       LeftRect;
	SDL_Rect       RightRect;

	SW_CLICK_RESULT   eLastClickResult;
	int            nPosClickX, nPosClickY;   //where a click started
	int            nVPosClickY, nHPosClickX; //where slider bar was when drag started
	bool           bIsUpButtonPressed, bIsDownButtonPressed;
	bool           bIsLeftButtonPressed, bIsRightButtonPressed;

	//Dimensions of areas drawn for widget.
	static const UINT CX_VBACK;   //vertical scroll bar
	static const UINT CY_VBACK;
	static const UINT CX_VPOS;
	static const UINT CY_VPOS;
	static const UINT CX_UP;
	static const UINT CY_UP;
	static const UINT CX_DOWN;
	static const UINT CY_DOWN;

	static const UINT CX_HBACK;   //horizontal scroll bar
	static const UINT CY_HBACK;
	static const UINT CX_HPOS;
	static const UINT CY_HPOS;
	static const UINT CX_LEFT;
	static const UINT CY_LEFT;
	static const UINT CX_RIGHT;
	static const UINT CY_RIGHT;
};

#endif //#ifndef SCROLLABLEWIDGET_H
