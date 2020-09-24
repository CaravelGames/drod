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

#include "Widget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include "Screen.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>

#include <SDL.h>

#include <list>

static SDL_Surface *m_pScreenSurface = NULL;

//***************************************************************************
void SetWidgetScreenSurface(
//Widgets will work after you say what the screen surface is.
//
//Params:
	SDL_Surface *pSetScreenSurface) //(in)
{
	ASSERT(pSetScreenSurface);
	m_pScreenSurface = pSetScreenSurface;
}

//***************************************************************************
SDL_Surface* GetWidgetScreenSurface()
//Returns the screen surface that is the default destination surface for
//all widgets.
//
//NOTE: Don't use this for drawing operations inside a widget.  That will work
//fine until we want the widget to draw on a non-screen surface.  Use the 
//surface returned from CWidget::GetDestSurface() instead.
{
	return m_pScreenSurface;
}

//
//CWidget protected methods.
//

//***************************************************************************
CWidget::CWidget(
//Constructor.
//
//Params:
	WIDGETTYPE eSetType, //(in)   Type of widget.
	UINT dwSetTagNo,    //(in)   Arbitrary ID for this widget.
	int nSetX, int nSetY,   //(in)   Rect that widget's paint area lies
	UINT wSetW, UINT wSetH) //    within.
	: bResizing(false)
	, bIsLoaded(false)
	, bIsEnabled(true)

	, x(nSetX)
	, y(nSetY)
	, w(wSetW)
	, h(wSetH)
	, nChildrenScrollOffsetX(0), nChildrenScrollOffsetY(0)
	, pParent(NULL)
	, bIsVisible(true)
	, dwTagNo(dwSetTagNo)
	, eType(eSetType)

	, pSurface(NULL)  //Destination surface is the screen.
{
	//Check for reserved tag usage.
	ASSERT(!IS_RESERVED_TAG(dwSetTagNo));
}

//***************************************************************************
CWidget::~CWidget()
//Destructor.
{
	ASSERT(!this->bIsLoaded);
	ClearChildren();
	RemoveAllHotkeys();
}

//*****************************************************************************
bool CWidget::CanResize() const
//Called whenever widget desires to resize itself.
{
	if (!this->pParent) return false;

	//Widgets in a scrollable widget container can make themselves as large as they like.
	if (this->pParent->eType == WT_Scrollable)
		return true;

	return false;
}

//***************************************************************************
void CWidget::ClearChildren()
//Clear all the widget children.
{
	//Delete the CWidgets.
	for (WIDGET_ITERATOR iSeek = this->Children.begin();
			iSeek != this->Children.end(); ++iSeek)
	{
		if ((*iSeek)->bIsLoaded) (*iSeek)->Unload();
		delete *iSeek;
	}

	//Clear the list.
	this->Children.clear();
}

//***************************************************************************
bool CWidget::ClipRectToRect(SDL_Rect& rect, const SDL_Rect& clipRect)
//Returns: whether part of 'rect' is inside 'clipRect'
{
	if (rect.x < clipRect.x)
	{
		//Clip left side of rect
		if (clipRect.x - rect.x >= static_cast<Sint16>(rect.w))
			return false;
		rect.w -= clipRect.x - rect.x;
		rect.x = clipRect.x;
	}
	const int pastRectRight = clipRect.x + clipRect.w;
	if (static_cast<Sint16>(rect.w) > pastRectRight - rect.x)
	{
		//Clip right side of rect
		if (rect.x >= pastRectRight)
			return false;
		rect.w = pastRectRight - rect.x;
	}
	if (rect.y < clipRect.y)
	{
		//Clip top of rect
		if (clipRect.y - rect.y >= static_cast<Sint16>(rect.h))
			return false;
		rect.h -= clipRect.y - rect.y;
		rect.y = clipRect.y;
	}
	const int pastRectBottom = clipRect.y + clipRect.h;
	if (static_cast<Sint16>(rect.h) > pastRectBottom - rect.y)
	{
		//Clip bottom of rect
		if (rect.y >= pastRectBottom)
			return false;
		rect.h = pastRectBottom - rect.y;
	}
	return true;
}

//*****************************************************************************
void CWidget::EraseChildren(
//Erases the area drawn over by the child widgets by repainting a given
//background in its place.
//Call when children are painted over a background that is otherwise never repainted,
//like over a CScreen that is normally painted only once.
//
//Params:
	SDL_Surface* pBackground, SDL_Rect rect,	//(in) background under effects
	const bool bUpdate)	//(in) [default=false]
{
	ASSERT(pBackground);

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);
	SDL_Rect ChildClipRect;
	GetChildClippingRect(ChildClipRect);

	SDL_Surface *pDestSurface = GetDestSurface();

	for (WIDGET_ITERATOR iSeek = this->Children.begin();
			iSeek != this->Children.end(); ++iSeek)
	{
		CWidget *pWidget = *iSeek;
		if (!pWidget->IsVisible()) continue;

		//Check for a completely out-of-bounds widget.
		if (!pWidget->OverlapsRect(ChildClipRect.x + nOffsetX, ChildClipRect.y + nOffsetY,
				ChildClipRect.w, ChildClipRect.h))
			continue;

		SDL_Rect dirty = MAKE_SDL_RECT(pWidget->x + nOffsetX, pWidget->y + nOffsetY, pWidget->w, pWidget->h);
		SDL_Rect srcRect = MAKE_SDL_RECT(rect.x + dirty.x, rect.y + dirty.y, dirty.w, dirty.h);
		SDL_BlitSurface(pBackground, &srcRect, pDestSurface, &dirty);
		if (bUpdate)
			UpdateRect(dirty);
	}
}

//***************************************************************************
void CWidget::GetScrollOffset(
//Gets the offset that should be applied to the position of this widget and
//its children.  If you add the offset to the x,y members, you will have the
//position of the widget on the screen.  The widget may be outside of the
//screen or the clipping area of its parent.
//
//Params:
	int &nOffsetX, int &nOffsetY) //(out)  Returns offsets.
const
{
	nOffsetX = nOffsetY = 0;

	//Add scroll offsets from all parents of this widget.
	const CWidget *pSeekParent = this->pParent;
	while (pSeekParent)
	{
		nOffsetX += pSeekParent->nChildrenScrollOffsetX;
		nOffsetY += pSeekParent->nChildrenScrollOffsetY;
		pSeekParent = pSeekParent->pParent;
	}
}

//***************************************************************************
bool CWidget::IsScrollOffset()
//Is there a scroll offset for this widget?  If caller is planning on using
//the offsets, just call GetScrollOffset() without calling this method.  Use
//this method if you just want to make a quick check.
//
//Returns:
//True if scrolling offsets are specified for any parent of this widget.
const
{
	const CWidget *pSeekParent = this->pParent;
	while (pSeekParent)
	{
		if (pSeekParent->nChildrenScrollOffsetX ||
				pSeekParent->nChildrenScrollOffsetY)
			return true; //Found at least one offset.
		pSeekParent = pSeekParent->pParent;
	}

	//No scrolling offsets found.
	return false;
}

//***************************************************************************
CWidget * CWidget::GetFirstSibling()
//Get the first sibling of this widget.
{
	if (!this->pParent) 
	{
		ASSERT(this->eType == WT_Screen);   //If fires, caller is probably trying
											//to get sibling of a widget that 
											//hasn't been added to a parent yet.
		return this;
	}

	ASSERT(this->pParent->Children.size());

	return *(this->pParent->Children.begin());
}

//***************************************************************************
CWidget * CWidget::GetLastSibling()
//Get the last sibling of this widget.
{
	if (!this->pParent) 
	{
		ASSERT(this->eType == WT_Screen);   //If fires, caller is probably trying
											//to get sibling of a widget that 
											//hasn't been added to a parent yet.
		return this;
	}

	ASSERT(this->pParent->Children.size());

	WIDGET_ITERATOR iRet = this->pParent->Children.end();
	return *(--iRet);
}

//***************************************************************************
CWidget * CWidget::GetPrevSibling()
//Get the previous sibling of this widget.
{
	if (!this->pParent) 
	{
		ASSERT(this->eType == WT_Screen);   //If fires, caller is probably trying
											//to get sibling of a widget that 
											//hasn't been added to a parent yet.
		return this;
	}

	ASSERT(this->pParent->Children.size());

	if (this->pParent->Children.size()==1) return this; //Widget is an only-child.

	for (WIDGET_ITERATOR iSeek = this->pParent->Children.begin(); 
			iSeek != this->pParent->Children.end(); ++iSeek)
	{
		if (*iSeek == this) //Found this widget in parent's child list.
		{
			//Get previous widget.
			if (iSeek == this->pParent->Children.begin())
				return NULL;
			return *(--iSeek);
		}
	}

	ASSERT(!"Parent's child list or this widget's parent pointer is incorrect.");
	return NULL;
}

//***************************************************************************
CWidget * CWidget::GetNextSibling()
//Get the next sibling of this widget.
{
	if (!this->pParent) 
	{
		ASSERT(this->eType == WT_Screen);   //If fires, caller is probably trying
											//to get sibling of a widget that 
											//hasn't been added to a parent yet.
		return this;
	}

	ASSERT(this->pParent->Children.size());

	if (this->pParent->Children.size()==1) return this; //Widget is an only-child.

	for (WIDGET_ITERATOR iSeek = this->pParent->Children.begin(); 
			iSeek != this->pParent->Children.end(); ++iSeek)
	{
		if (*iSeek == this) //Found this widget in parent's child list.
		{
			//Get next widget.
			if (++iSeek == this->pParent->Children.end())
				return NULL;
			return *iSeek;
		}
	}

	ASSERT(!"Parent's child list or this widget's parent pointer is incorrect.");
	return NULL;
}

//***************************************************************************
CEventHandlerWidget * CWidget::GetParentEventHandlerWidget() const
{
	if (!this->pParent) 
		return NULL; //This widget doesn't have an event handler.

	return this->pParent->GetEventHandlerWidget();
}

//***************************************************************************
CEventHandlerWidget * CWidget::GetEventHandlerWidget()
//Find event-handler widget that handles events for this widget.
//
//Returns:
//Pointer to a event-handling widget or NULL if none found.
{
	//Check against known list of widget types that are event handlers.
	if (
			this->eType == WT_Screen ||
			this->eType == WT_Dialog)
	{
		//This widget is an event-handler.
		//We need a dynamic cast instead of a regular cast here because the cast
		//will fail if this is called by the destructor of one of the superclasses
		//of CEventHandlerWidget.
		return dynamic_cast<CEventHandlerWidget*>(this);
	}

	//Check parent(s).
	return GetParentEventHandlerWidget();
}

//***************************************************************************
void CWidget::SetParent(
//Sets parent widget of this widget.
//
//Params:
	CWidget *const pSetParent) //(in)
{
	ASSERT(pSetParent);
	this->pParent = pSetParent;
}

//***************************************************************************
void CWidget::PaintChildren(
//Paint all the visible child widgets.
//
//Params:
	bool bUpdateRects)   //(in)   If true, the SDL_UpdateRect will be called
						//    for each painted widget.  This will immediately
						//    update the screen if destination surface is
						//    pointing to it.  Default is false.
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);
	SDL_Rect ChildClipRect;
	GetChildClippingRect(ChildClipRect);

	for (WIDGET_ITERATOR iSeek = this->Children.begin();
			iSeek != this->Children.end(); ++iSeek)
	{
		CWidget *pWidget = *iSeek;
		if (pWidget->IsVisible())
		{
			//Check for a completely out-of-bounds widget.
			if (!pWidget->OverlapsRect(ChildClipRect.x + nOffsetX, ChildClipRect.y + nOffsetY,
					ChildClipRect.w, ChildClipRect.h))
				continue;

			//Check for clipping.
			if (pWidget->IsInsideOfRect(ChildClipRect.x + nOffsetX,
					ChildClipRect.y + nOffsetY, ChildClipRect.w, ChildClipRect.h))
				pWidget->Paint(bUpdateRects);
			else
				pWidget->PaintClipped(ChildClipRect.x + nOffsetX, ChildClipRect.y + nOffsetY, 
						ChildClipRect.w, ChildClipRect.h, bUpdateRects);
		}
	}
}

//*****************************************************************************
bool CWidget::Load()
//Overridable method that loads resources for the widget.
//
//Returns:
//True if successful, false if not.
{
	ASSERT(!this->bIsLoaded);

	for (UINT wIndex=0; wIndex<this->imageFilenames.size(); ++wIndex)
	{
		//Load a graphics bitmap.
		ASSERT(this->images.size() <= wIndex || !this->images[wIndex]);
		SDL_Surface *pSurface = g_pTheBM->GetBitmapSurface(
				this->imageFilenames[wIndex].c_str());
		if (!pSurface)
			return false;
		this->images.push_back(pSurface);
	}

	this->bIsLoaded = LoadChildren();
	
	return this->bIsLoaded;
}

//*****************************************************************************
void CWidget::Unload()
//Overridable method that unloads resources for the widget.
{
	ASSERT(this->bIsLoaded);
	
	UnloadChildren();

	for (UINT wIndex=0; wIndex<this->imageFilenames.size(); ++wIndex)
	{
		if (this->images[wIndex])
		{
			g_pTheBM->ReleaseBitmapSurface(this->imageFilenames[wIndex].c_str());
		}
	}
	this->images.clear();

	this->bIsLoaded = false;
}

//***************************************************************************
bool CWidget::LoadChildren()
//Load all the widget children.
//
//Returns:
//True if all children were loaded, false if one load failed.
{
	//Load the CWidgets.
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		if (!(*iSeek)->IsLoaded())
		{
			if (!(*iSeek)->Load())
				return false;
			ASSERT((*iSeek)->IsLoaded()); //Check that derived classes 
										//are setting bIsLoaded.
		}
	}

	//All widgets loaded.
	return true;
}

//***************************************************************************
void CWidget::UnloadChildren()
//Unload all the widget children.
{
	//Unload the CWidgets.
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		if ((*iSeek)->IsLoaded())
		{
			(*iSeek)->Unload();
			ASSERT(!(*iSeek)->IsLoaded());   //Check that derived classes 
											//are clearing bIsLoaded.
		}
	}
}

//
//CWidget public methods.
//

#define PPIXEL(col,row) ( (Uint8 *)pDestSurface->pixels + \
	((row) * pDestSurface->pitch) + \
	((col) * pDestSurface->format->BytesPerPixel) )

//***************************************************************************
void CWidget::Scroll(
//Sets offset that children are moved by when painted.  Used for scrolling
//children inside of the widget.  If children will be moving outside of widget
//area, you should make sure that all children have a working PaintClipped()
//method.
//
//The return parameters values of GetScrollOffset() for children of this 
//widget will be affected by this call.
//
//Params:
	int dx,        //(in)   Offsets to apply to children.
	int dy)        //    Use negative numbers for left
					//    and up offsets.
{
	this->nChildrenScrollOffsetX += dx;
	this->nChildrenScrollOffsetY += dy;
}

//***************************************************************************
void CWidget::ScrollAbsolute(
//Sets offset that children are moved by when painted.  Used for scrolling
//children inside of the widget.  If children will be moving outside of widget
//area, you should make sure that all children have a working PaintClipped()
//method.
//
//The return parameters values of GetScrollOffset() for children of this 
//widget will be affected by this call.
//
//Params:
	int nSetOffsetX,  //(in)   Offsets to apply to children.  
	int nSetOffsetY)  //    Use negative numbers for left
						//    and up offsets.  0,0 removes all offsets.
{
	this->nChildrenScrollOffsetX = nSetOffsetX;
	this->nChildrenScrollOffsetY = nSetOffsetY;
}

//***************************************************************************
void CWidget::SetHeight(const UINT wHeight)
//Update one dimension of the widget's size.
{
	Resize(this->w, wHeight);
}

void CWidget::SetWidth(const UINT wWidth)
{
	Resize(wWidth, this->h);
}

//***************************************************************************
bool CWidget::GetBlitRectFromClipRect(
//Given a location to draw and a clipping rect,
//update a blitting rectangle to lie entirely within the clipping rect.
//
//Returns: whether any part of the destination is within the clipping rect
//
//Params:
	const int nPixelX, const int nPixelY,  //(in/out) location to draw
	const SDL_Rect& ClipRect, //(in) rect to clip against
	SDL_Rect& BlitRect) //(in/out) src/dest image rect to draw
{
	if (nPixelX < ClipRect.x)
	{
		BlitRect.x = ClipRect.x - nPixelX;
		ASSERT(BlitRect.x > 0);
		if (BlitRect.x >= BlitRect.w)
			return false; //completely outside rect
		BlitRect.w -= BlitRect.x;
	}
	else if (nPixelX + (int)BlitRect.w >= ClipRect.x + (int)ClipRect.w)
	{
		if (nPixelX >= ClipRect.x + (int)ClipRect.w)
			return false; //completely outside rect
		BlitRect.w = (ClipRect.x + ClipRect.w) - nPixelX;
	}
	if (nPixelY < ClipRect.y)
	{
		BlitRect.y = ClipRect.y - nPixelY;
		ASSERT(BlitRect.y > 0);
		if (BlitRect.y >= BlitRect.h)
			return false; //completely outside rect
		BlitRect.h -= BlitRect.y;
	}
	else if (nPixelY + (int)BlitRect.h >= ClipRect.y + (int)ClipRect.h)
	{
		if (nPixelY >= ClipRect.y + (int)ClipRect.h)
			return false; //completely outside rect
		BlitRect.h = (ClipRect.y + ClipRect.h) - nPixelY;
	}
	return true;
}

//***************************************************************************
void CWidget::GetRectContainingChildren(
//Gets the smallest rect that will contain all children of this widget.
//
//Params:
	SDL_Rect &ChildContainerRect) //(out)
const
{
	//Is there at least one widget?
	WIDGET_ITERATOR iSeek = this->Children.begin();
	SDL_Rect ChildRect;
	if (iSeek == this->Children.end())
	{
		//No--Set the container to zero width/depth and return early.
		SET_RECT(ChildContainerRect, 0, 0, 0, 0);
		return;
	}
	else
	{
		//Yes--Set the container to the first child.
		(*iSeek)->GetRect(ChildContainerRect);
	}

	//For any remaining children, expand boundaries of container to include them.
	for (++iSeek; //Advance to second child.
			iSeek != this->Children.end(); ++iSeek)
	{
		(*iSeek)->GetRect(ChildRect);

		//Expand left of container to include child if needed.
		const int nDiffX = ChildContainerRect.x - ChildRect.x;
		if (nDiffX > 0)
		{
			ChildContainerRect.x -= nDiffX;
			ChildContainerRect.w += nDiffX;
		}

		//Expand top of container to include child if needed.
		const int nDiffY = ChildContainerRect.y - ChildRect.y;
		if (nDiffY > 0)
		{
			ChildContainerRect.y -= nDiffY;
			ChildContainerRect.h += nDiffY;
		}

		//Expand right of container to include child if needed.
		const int nChildRightX = ChildRect.x + ChildRect.w;
		const int nContainerRightX = ChildContainerRect.x + ChildContainerRect.w;
		if (nChildRightX > nContainerRightX)
			ChildContainerRect.w += (nChildRightX - nContainerRightX);

		//Expand bottom of container to include child if needed.
		const int nChildBottomY = ChildRect.y + ChildRect.h;
		const int nContainerBottomY = ChildContainerRect.y + ChildContainerRect.h;
		if (nChildBottomY > nContainerBottomY)
			ChildContainerRect.h += (nChildBottomY - nContainerBottomY);
	}

	//Take child widget offset into account.
	ChildContainerRect.x += this->nChildrenScrollOffsetX;
	ChildContainerRect.y += this->nChildrenScrollOffsetY;
}

//***************************************************************************
void CWidget::HideChildren()
//Hides all children of this widget, but not the widget itself.  If you wish
//to hide both, simply call Hide().
{
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		(*iSeek)->Hide();
	}
}

//***************************************************************************
void CWidget::ShowChildren()
//Shows all children of this widget, but not the widget itself.  If you wish
//to show both, simply call Show().
{
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		(*iSeek)->Show();
	}
}

//***************************************************************************
void CWidget::DrawPlaceholder()
//Draws a solid rect over the widget area.
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);
	SURFACECOLOR Color = GetSurfaceColor(GetDestSurface(), RGB_PLACEHOLDER);
	SDL_Rect rect = MAKE_SDL_RECT(this->x + nOffsetX, this->y + nOffsetY, this->w, this->h);
	DrawFilledRect(rect, Color);
}

//*****************************************************************************
void CWidget::Center()
//Centers this widget inside its parent area.
{
	ASSERT(this->pParent);
	const int nMoveX = (static_cast<int>(this->pParent->w) - static_cast<int>(this->w)) / 2;
	const int nMoveY = (static_cast<int>(this->pParent->h) - static_cast<int>(this->h)) / 2;
	Move(nMoveX, nMoveY);
}

//*****************************************************************************
bool CWidget::ContainsCoords(
//Does the widget's area contain specified coordinates?
//
//Returns:
//True if it does, false if not.
//
//Params:
	const int nX, const int nY) //(in) Coords to compair against.
const
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	return (nX >= this->x + nOffsetX && nY >= this->y + nOffsetY &&
			nX < this->x + nOffsetX + static_cast<int>(this->w) && nY < this->y +
			nOffsetY + static_cast<int>(this->h));
}

//*****************************************************************************
bool CWidget::IsInsideOfParent()
//Is the widget's area completely inside of its parent?
//
//Returns:
//True if it is, false if not.
const
{
	if (!this->pParent)
		return true;

	SDL_Rect ParentRect;
	this->pParent->GetRect(ParentRect);
	return IsInsideOfRect(ParentRect.x, ParentRect.y, 
			ParentRect.w, ParentRect.h);
}

//*****************************************************************************
bool CWidget::IsInsideOfParents() const
//Is the widget's area completely inside of all parent widgets?
{
	for (CWidget *pParent = this->pParent; pParent != NULL; pParent = pParent->pParent)
	{
		SDL_Rect ParentRect;
		pParent->GetRect(ParentRect);
		if (!IsInsideOfRect(ParentRect.x, ParentRect.y, ParentRect.w, ParentRect.h))
			return false;
	}
	return true;
}

//*****************************************************************************
bool CWidget::IsSelectable(
//Returns: whether a widget can receive focus
//
//Params:
	const bool bSelectIfParentIsHidden) //[default=false]
const
{
	if (!IsEnabled() || !IsVisible() || !IsFocusable())
		return false;

	//If a parent is invisible, don't allow focus on me either.
	if (!bSelectIfParentIsHidden)
		for (CWidget *pParent = this->pParent; pParent != NULL; pParent = pParent->pParent)
			if (!pParent->IsVisible())
				return false;

	return true;
}

//*****************************************************************************
bool CWidget::OverlapsRect(
//Does any part of this widget overlap a specified rectangle?
//
//Params:
	const int nX, const int nY,
	const UINT wW, const UINT wH) //(in) Rect to compare against.
//
//Returns:
//True if it does, false if not.
const
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	return (!(
	
			//Rect is too far left to overlap.
			(int)(nX + wW) < this->x + nOffsetX ||

			//Rect is too far right.
			nX > (int)(this->x + nOffsetX + this->w) ||

			//Rect is too far up.
			(int)(nY + wH) < this->y + nOffsetY ||

			//Rect is too far down.
			nY > (int)(this->y + nOffsetY + this->h)) );
}

//*****************************************************************************
bool CWidget::IsInsideOfRect(
//Is the widget's area completely inside of a specified rectangle?
//
//Params:
	int nX, int nY, UINT wW, UINT wH)   //(in) Rect to compare against.
//
//Returns:
//True if it is, false if not.
const
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	if (this->x + nOffsetX < nX ||
			this->x + nOffsetX + this->w > nX + wW ||
			this->y + nOffsetY < nY ||
			this->y + nOffsetY + this->h > nY + wH)
		return false;
	else
		return true;
}
	
//*****************************************************************************
void CWidget::PaintClipped(
//Paint widget inside of a clipped area.  PaintChildren() will automatically
//call this for children with rects outside of the widget area.  You should
//override this method if your widget uses direct pixel access and is expected
//to be clipped.
//
//Params:
	const int nX, const int nY,   //(in)   The rect to clip using screen coords.
	const UINT wW, const UINT wH, //(in)   The rect to clip using screen coords.
	const bool bUpdateRect)       //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	SDL_Surface *pDestSurface = GetDestSurface();
	SDL_Rect ClipRect = MAKE_SDL_RECT(nX, nY, wW, wH);
	SDL_SetClipRect(pDestSurface, &ClipRect);
	Paint(bUpdateRect);
	SDL_SetClipRect(pDestSurface, NULL);
}

//*****************************************************************************
void CWidget::PaintClippedInsideParent(
//Calls PaintClipped() with parent widget rect.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	PaintClipped(pParent->x, pParent->y, pParent->w, pParent->h, bUpdateRect);
}

//*****************************************************************************
void CWidget::GetChildClippingRect(
//Output the area that this widget wants its children to be drawn within.
//
//Params:
	SDL_Rect &ChildClipRect)   //(out) the area this widget wants its children clipped to
const
{
	//Just clip to my area.
	ChildClipRect.x = this->x;
	ChildClipRect.y = this->y;
	ChildClipRect.w = this->w;
	ChildClipRect.h = this->h;
}

//*****************************************************************************
SDL_Surface * CWidget::GetDestSurface() const
//Get surface that should be used for all drawing operations involving this
//widget.
{
	//If surface is NULL, return the screen surface, otherwise return the
	//explicitly set surface.
	return (this->pSurface == NULL) ?
		m_pScreenSurface : this->pSurface;
}

//*****************************************************************************
void CWidget::SetDestSurface(
//Set surface that should be used for all drawing operations involving this
//widget.  Child widgets will have their surfaces set as well.
//
//Params:
	SDL_Surface *pSetSurface)  //(in)   New destination surface.  NULL indicates 
								//    that screen surface will be used.
{
	//Set this widget's dest surface (and children).
	//If the surface being set is the screen surface, value should be NULL.
	this->pSurface = pSetSurface == m_pScreenSurface ? NULL : pSetSurface;
	SetChildrenDestSurface(pSetSurface);
}

//*****************************************************************************
void CWidget::SetChildrenDestSurface(
//Same as SetDestSurface(), but only changes dest surface of children.
//
//Params:
	SDL_Surface *pSetSurface)  //(in)   New destination surface.  NULL indicates 
								//    that screen surface will be used.
{
	//Set children.
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
				iSeek != this->Children.end(); ++iSeek)
		(*iSeek)->SetDestSurface(pSetSurface);
}

//*****************************************************************************
void CWidget::UpdateRect() const
//Updates dest surface for this widget's rect.
{
	int nUpdateX, nUpdateY;
	GetScrollOffset(nUpdateX, nUpdateY);
	nUpdateX += this->x;
	nUpdateY += this->y;

	UpdateRect(nUpdateX, nUpdateY, this->w, this->h);
}

//*****************************************************************************
void CWidget::UpdateRect(SDL_Rect &rect) const
{
	//Bounds checks.  Crop update to screen area.
	if (rect.x < 0)
	{
		rect.w += rect.x;
		rect.x = 0;
	}
	if (rect.y < 0)
	{
		rect.h += rect.y;
		rect.y = 0;
	}
	if (static_cast<UINT>(rect.x + rect.w) > static_cast<UINT>(CScreen::CX_SCREEN))
		rect.w = CScreen::CX_SCREEN - rect.x;
	if (static_cast<UINT>(rect.y + rect.h) > static_cast<UINT>(CScreen::CY_SCREEN))
		rect.h = CScreen::CY_SCREEN - rect.y;

	g_pTheBM->UpdateRect(rect);
}

//*****************************************************************************
void CWidget::Move(
//
//Params:
	const int nSetX, const int nSetY)   //(in)
{
	int dx, dy;
	if (this->pParent)
	{
		dx = nSetX - (this->x - this->pParent->x);
		dy = nSetY - (this->y - this->pParent->y);
	}
	else
	{
		dx = nSetX - this->x;
		dy = nSetY - this->y;
	}
	this->x += dx; 
	this->y += dy;
	
	//Move children.
	MoveChildren(dx, dy);
}

//*****************************************************************************
void CWidget::MoveChildren(const int dx, const int dy)
{
	if (dx || dy)
	{
		for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
				iSeek != this->Children.end(); ++iSeek)
		{
			(*iSeek)->Move((*iSeek)->x - this->x + dx, 
					(*iSeek)->y - this->y + dy);
		}
	}
}

//*****************************************************************************
void CWidget::RequestPaint(const bool bUpdateRect)
//Wrapper method to Paint().  Call this method to request calls to Paint().
//It will ensure that child widgets are clipped properly to the area desired by
//the parent, if the parent desires to enforce this.
{
	if (g_pTheSM->bTransitioning) return;

	if (this->pParent && this->pParent->ParentMustPaintChildren())
	{
		SDL_Rect ChildClipRect;
		this->pParent->GetChildClippingRect(ChildClipRect);

		//Check for a completely out-of-bounds widget.
		if (!OverlapsRect(ChildClipRect.x, ChildClipRect.y,
				ChildClipRect.w, ChildClipRect.h))
			return;

		//Check for clipping.
		if (IsInsideOfRect(ChildClipRect.x,
				ChildClipRect.y, ChildClipRect.w, ChildClipRect.h))
			Paint(bUpdateRect);
		else
			PaintClipped(ChildClipRect.x, ChildClipRect.y, 
					ChildClipRect.w, ChildClipRect.h, bUpdateRect);
		return;
	}

	Paint(bUpdateRect);
}

//*****************************************************************************
void CWidget::Resize(
//
//Params:
	const UINT wSetW, const UINT wSetH) //(in)
{
	if (wSetW == this->w && wSetH == this->h) return;
	this->w = wSetW;
	this->h = wSetH;

	//Don't allow recursive calls to ChildResized(),
	//since ChildResized() might in turn call Resize() on this widget.
	if (this->bResizing) return;
	this->bResizing = true;

	if (this->pParent)
		this->pParent->ChildResized();

	this->bResizing = false;
}

//*****************************************************************************
void CWidget::DrawFilledRect(
//Draws a filled rectangle to the destination surface.
//
//Params:
	SDL_Rect &rect,      //(in)
	const SURFACECOLOR &Color, //(in)
	SDL_Surface *pDestSurface) //(in) [default = NULL]
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	//Clip rectangle to clipping rectangle, if needed.
	SDL_Rect clipRect;
	SDL_GetClipRect(pDestSurface, &clipRect);
	if (!ClipRectToRect(rect, clipRect))
		return;

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);

	//Calc offset between end of a row and beginning of next.
	const UINT dwRowOffset = pDestSurface->pitch - (rect.w * wBPP);

	//Calc location of top-left pixel.
	Uint8 *pSeek = (Uint8 *)(pDestSurface->pixels) + 
			(rect.y * pDestSurface->pitch) + (rect.x * wBPP);
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	pSeek += wBPP-3;  // a crude hack.  when wBPP==4, offset by one byte
#endif

	//Calc location of bottom-left pixel plus one row too far.
	const Uint8 *pEndOfSeek = pSeek + (rect.h * pDestSurface->pitch);

	//Lock surface.
	LockDestSurface(pDestSurface);

	//Each iteration fills one row of pixels.
	while (pSeek != pEndOfSeek)
	{
		ASSERT(pSeek < pEndOfSeek);
		const Uint8 *pEndOfRow = pSeek + (rect.w * wBPP);

		//Each iteration sets 3 bytes (1 pixel).
		while (pSeek != pEndOfRow)
		{
			pSeek[0] = Color.byt1;
			pSeek[1] = Color.byt2;
			pSeek[2] = Color.byt3;
			pSeek += wBPP;
		}

		//Advance to beginning of next row.
		pSeek += dwRowOffset;
	}

	//Unlock surface.
	UnlockDestSurface(pDestSurface);
}

//*****************************************************************************
void CWidget::DrawRect(
//Draws a rectangle (unfilled) to the dest surface.
//
//Params:
	const SDL_Rect &rect,      //(in)
	const SURFACECOLOR &Color, //(in)
	SDL_Surface *pDestSurface) //(in) [default = NULL]
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	//Clip rectangle to clipping rectangle, if needed.
	SDL_Rect clipRect;
	SDL_GetClipRect(pDestSurface, &clipRect);
	if (  rect.x < clipRect.x ||
			rect.y < clipRect.y ||
			rect.x + rect.w > clipRect.x + clipRect.w ||
			rect.y + rect.h > clipRect.y + clipRect.h)
	{
		DrawRectClipped(rect, Color, pDestSurface);
		return;
	}

	static UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);

	LockDestSurface(pDestSurface);

	//Draw top row.
	Uint8 *pSeek = PPIXEL(rect.x, rect.y);
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	pSeek += wBPP-3;  // a crude hack.  when wBPP==4, offset by one byte
#endif
	Uint8 *pStop = pSeek + (rect.w * wBPP);
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		pSeek[0] = Color.byt1;
		pSeek[1] = Color.byt2;
		pSeek[2] = Color.byt3;
		pSeek += wBPP;
	}

	if (rect.h >= 2)
	{
		//Draw bottom row.
		pSeek = PPIXEL(rect.x, rect.y + rect.h - 1);
#ifdef GAME_RENDERING_OFFSET
		pSeek += wBPP-3;
#endif
		pStop = pSeek + (rect.w * wBPP);
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			pSeek[0] = Color.byt1;
			pSeek[1] = Color.byt2;
			pSeek[2] = Color.byt3;
			pSeek += wBPP;
		}
		
		//Draw left column.
		pSeek = PPIXEL(rect.x, rect.y + 1);
#ifdef GAME_RENDERING_OFFSET
		pSeek += wBPP-3;
#endif
		pStop = pSeek + (rect.h - 2) * pDestSurface->pitch;
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			pSeek[0] = Color.byt1;
			pSeek[1] = Color.byt2;
			pSeek[2] = Color.byt3;
			pSeek += pDestSurface->pitch;
		}
		
		//Draw right column.
		pSeek = PPIXEL(rect.x + rect.w - 1, rect.y + 1);
#ifdef GAME_RENDERING_OFFSET
		pSeek += wBPP-3;
#endif
		pStop = pSeek + (rect.h - 2) * pDestSurface->pitch;
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			pSeek[0] = Color.byt1;
			pSeek[1] = Color.byt2;
			pSeek[2] = Color.byt3;
			pSeek += pDestSurface->pitch;
		}
	}

	//Unlock screen.
	UnlockDestSurface(pDestSurface);
}

//*****************************************************************************
void CWidget::DrawRectClipped(
//Draws a clipped rectangle (unfilled) to the dest surface.
//
//Params:
	const SDL_Rect &rect,      //(in)
	const SURFACECOLOR &Color, //(in)
	SDL_Surface *pDestSurface) //(in)
{
	ASSERT(pDestSurface);

	//These functions will perform the line clipping.
	DrawRow(rect.x, rect.y, rect.w, Color, pDestSurface);
	if (rect.h >= 2)
	{
		DrawRow(rect.x, rect.y + rect.h - 1, rect.w, Color, pDestSurface);
		DrawCol(rect.x, rect.y + 1, rect.h - 2, Color, pDestSurface);
		DrawCol(rect.x + rect.w - 1, rect.y + 1, rect.h - 2, Color, pDestSurface);
	}
}

//*****************************************************************************
void CWidget::DrawCol(
//Draws a column (vertical line) to the screen.
//
//Params:
	int nX, int nY,            //(in)   Coords for top of column.
	UINT wH,             //(in)   Height of column.
	const SURFACECOLOR &Color, //(in)   Color of column pixels.
	SDL_Surface *pDestSurface) //(in)  default == NULL
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	//Clip line to clipping rectangle, if needed.
	SDL_Rect clipRect;
	SDL_GetClipRect(pDestSurface, &clipRect);
	if (nX < clipRect.x)
		return;  //line is outside clip rectangle
	if (nX >= clipRect.x + clipRect.w)
		return;
	if (nY < clipRect.y)
	{
		//Clip top of line
		if (clipRect.y - nY >= static_cast<int>(wH)) return; 
		wH -= clipRect.y - nY;
		nY = clipRect.y;
	}
	const int pastRectBottom = clipRect.y + clipRect.h;
	if (static_cast<int>(wH) > pastRectBottom - nY)
	{
		//Clip bottom of line
		if (nY >= pastRectBottom) return;
		wH = pastRectBottom - nY;
	}

	LockDestSurface(pDestSurface);

	//Draw column.
	Uint8 *pSeek = PPIXEL(nX, nY);
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	pSeek += wBPP-3;  // a crude hack.  when wBPP==4, offset by one byte
#endif
	Uint8 *pStop = pSeek + (pDestSurface->pitch * wH);
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		pSeek[0] = Color.byt1;
		pSeek[1] = Color.byt2;
		pSeek[2] = Color.byt3;
		pSeek += pDestSurface->pitch;
	}

	UnlockDestSurface(pDestSurface);
}

//*****************************************************************************
void CWidget::DrawRow(
//Draws a row (horizontal line) to the screen.
//
//Params:
	int nX, int nY,            //(in)   Coords for left of row.
	UINT wW,             //(in)   Width of row.
	const SURFACECOLOR &Color, //(in)   Color of row pixels.
	SDL_Surface *pDestSurface) //(in)  default == NULL
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	//Clip line to clipping rectangle, if needed.
	SDL_Rect clipRect;
	SDL_GetClipRect(pDestSurface, &clipRect);
	if (nY < clipRect.y)
		return;  //line is outside clip rectangle
	if (nY >= clipRect.y + clipRect.h)
		return;
	if (nX < clipRect.x)
	{
		//Clip left of line
		if (clipRect.x - nX >= static_cast<int>(wW)) return; 
		wW -= clipRect.x - nX;
		nX = clipRect.x;
	}
	const int pastRectRight = clipRect.x + clipRect.w;
	if (wW > UINT(pastRectRight - nX))
	{
		//Clip right of line
		if (nX >= pastRectRight) return;
		wW = pastRectRight - nX;
	}

	LockDestSurface(pDestSurface);

	//Draw column.
	Uint8 *pSeek = PPIXEL(nX, nY);
	static UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	pSeek += wBPP-3;  // a crude hack.  when wBPP==4, offset by one byte
#endif
	Uint8 *pStop = pSeek + (wW * wBPP);
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		pSeek[0] = Color.byt1;
		pSeek[1] = Color.byt2;
		pSeek[2] = Color.byt3;
		pSeek += wBPP;
	}

	UnlockDestSurface(pDestSurface);
}

//*****************************************************************************
void CWidget::DrawPixel(
//Draws a pixel to the screen.
//Calling this repeatedly is inefficient -- use DrawCol/Row for line drawing.
//
//Params:
	const int nX, const int nY,            //(in)   Pixel Coords.
	const SURFACECOLOR &Color, //(in)   Color of row pixels.
	SDL_Surface *pDestSurface) //(in)  default == NULL
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	//Clip to clipping rectangle, if needed.
	SDL_Rect clipRect;
	SDL_GetClipRect(pDestSurface, &clipRect);
	if (nX < clipRect.x)
		return;
	if (nY < clipRect.y)
		return;  //line is outside clip rectangle
	if (nX >= clipRect.x + clipRect.w)
		return;
	if (nY >= clipRect.y + clipRect.h)
		return;

	LockDestSurface(pDestSurface);

	//Draw pixel.
	Uint8 *pSeek = PPIXEL(nX, nY);
	pSeek[0] = Color.byt1;
	pSeek[1] = Color.byt2;
	pSeek[2] = Color.byt3;

	UnlockDestSurface(pDestSurface);
}

//***************************************************************************
void CWidget::GetDestSurfaceColor(
//Gets bytes that can be written to destination surface to produce a pixel of 
//specified color.
//
//Params:
	Uint8 bytRed,     //(in)   Desired color.
	Uint8 bytGreen,      //
	Uint8 bytBlue,    //
	SURFACECOLOR &Color)//(out)   Returns 3 bytes in the structure.
const
{
	Color = GetSurfaceColor(GetDestSurface(), bytRed, bytGreen, bytBlue);
}

//***************************************************************************
CWidget *CWidget::AddWidget(
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
	ASSERT(pWidget);

	ASSERT(pWidget->GetTagNo() == 0 || pWidget->GetTagNo() == TAG_OK ||
			GetWidget(pWidget->GetTagNo()) == NULL);

	//Add focusable or animated widgets to lists.
	if (pWidget->IsFocusable() || pWidget->IsAnimated())
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) 
		{
			if (pWidget->IsFocusable())
				pEventHandler->AddFocusWidget(pWidget);
			if (pWidget->IsAnimated())
				pEventHandler->AddAnimatedWidget(pWidget);
		}
	}

	//Move child widget relative to parent.
	pWidget->SetParent(this);
	pWidget->Move(pWidget->x, pWidget->y);
	this->Children.push_back(pWidget);

	if (bLoad && !pWidget->Load()) 
		return NULL;
	return pWidget;
}

//***************************************************************************
void CWidget::RemoveWidget(
//Remove a child widget from this widget.
//
//Params:
	CWidget *pRemoveWidget)
{
	ASSERT(GetWidget(pRemoveWidget->GetTagNo()) != NULL); //Is widget a child?

	//Remove focusable or animated widgets from lists.
	if (pRemoveWidget->IsFocusable() || pRemoveWidget->IsAnimated())
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) 
		{
			if (pRemoveWidget->IsFocusable())
				pEventHandler->RemoveFocusWidget(pRemoveWidget);
			if (pRemoveWidget->IsAnimated())
				pEventHandler->RemoveAnimatedWidget(pRemoveWidget);
		}
	}

	//Remove widget from child list.
	this->Children.remove(pRemoveWidget);

	//Free the widget and children's resources.
	if (pRemoveWidget->IsLoaded())
		pRemoveWidget->Unload();
	delete pRemoveWidget;
}

//***************************************************************************
CWidget* CWidget::GetWidget(
//Get a specific widget.
//
//Params:
	const UINT dwFindTagNo,   //(in)   Identifies widget to get.
	const bool bFindVisibleOnly)  //(in)   Widget must be visible (default=false).
{
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		if (!bFindVisibleOnly || (*iSeek)->IsVisible())
		{
			if ((*iSeek)->GetTagNo() == dwFindTagNo)
				return *iSeek; //Found it.

			//Look recursively for the tag.
			CWidget *pFromChild = (*iSeek)->GetWidget(dwFindTagNo);
			if (pFromChild)
				return pFromChild;
		}
	}

	//No match.
	return NULL;
}

//***************************************************************************
CWidget* CWidget::GetWidgetContainingCoords(
//Gets widget that contains coords.  Recursively called on child widgets
//to get the widget with a smaller area containing the coords.  If two or more
//widget siblings contain the coords, then the first one found will be returned.
//Hidden and disabled widgets are ignored, as well as children clicked outside
//the parent's clipping region.
//
//Params:
	const int nX, const int nY,         //(in)   Coords to check against.
	const WIDGETTYPE eWidgetType) //(in)   Type of widget required for a match.  If
							//    WT_Unspecified, criteria will not be applied.
//
//Returns:
//Found widget or NULL if no match.
const
{
	//Each iteration looks at one child of this widget for a match.
	for (WIDGET_ITERATOR iSeek = this->Children.begin(); 
			iSeek != this->Children.end(); ++iSeek)
	{
		CWidget *pWidget = *iSeek;
		//Filter by visibility.
		if (!pWidget->bIsVisible || !pWidget->bIsEnabled) continue;

		//Check coords against widget.
		if (pWidget->ContainsCoords(nX, nY)) //Found a match.
		{
			//If click is outside region where child widgets are being drawn,
			//then don't pass click through to them.
			SDL_Rect ClipRect;
			pWidget->GetChildClippingRect(ClipRect);
			if (!(nX >= ClipRect.x && nY >= ClipRect.y &&
					nX < ClipRect.x + ClipRect.w &&
					nY < ClipRect.y + ClipRect.h))
				return pWidget;

			//Look recursively for an even closer match.
			CWidget *pFromChild = pWidget->GetWidgetContainingCoords(nX, nY,
					eWidgetType);
			if (pFromChild &&
					(eWidgetType == WT_Unspecified || pFromChild->eType == eWidgetType) )
				return pFromChild;   //Matching child widget.
			if ((eWidgetType == WT_Unspecified || pWidget->eType == eWidgetType) )
				return pWidget;      //No matching child widget so use this one.
		}
	}

	//No match.
	return NULL;
}

//*****************************************************************************
Uint32 CWidget::IsLocked() const
//Whether surface is locked.
{
	return GetDestSurface()->locked;
}

//*****************************************************************************
SDL_Surface *CWidget::LockDestSurface(SDL_Surface *pDestSurface) //(in) [default = NULL])
//Simplified call to SDL_LockSurface().
//
//Returns:
//The dest surface.  Caller could have also gotten it from GetDestSurface(), 
//but it is returned here for convenience.
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();
	if (SDL_MUSTLOCK(pDestSurface)) 
	{
		while ( SDL_LockSurface(pDestSurface) < 0 ) 
			SDL_Delay(100);
	}
	return pDestSurface;
}

//*****************************************************************************
void CWidget::UnlockDestSurface(SDL_Surface *pDestSurface) //(in) [default = NULL]
//Simplified call to SDL_UnlockSurface().
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();
	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//*****************************************************************************
WCHAR CWidget::TranslateUnicodeKeysym(const SDL_Keysym& keysym, const bool bConsiderCaps) //[true]
//If Unicode value is not correct, then returns a corrected value of the keysym's
//unicode field according to a hardcoded table of values.
{
	// XXX temporary hack. we have to use sdl's text input events to fix input properly.

	WCHAR wc = W_t((keysym.sym > 0 && keysym.sym < 0x7f) ? keysym.sym : 0);

	//Determine capitalization.
	bool bCaps = false;
	if (bConsiderCaps)
	{
		bCaps = (keysym.mod & KMOD_SHIFT) != 0;
		if (keysym.mod & KMOD_CAPS)
			bCaps = !bCaps;
	}

	TranslateUnicodeKeysym(wc, keysym.sym, bCaps);

	return wc;
}

//*****************************************************************************
void CWidget::TranslateUnicodeKeysym(WCHAR& wc, const SDL_Keycode sym, const bool bCaps)
{
	UINT wc_i=WCv(wc);
	//Hardcoded keyboard lookup for specific languages.
	switch (Language::GetLanguage())
	{
		default:
		case Language::English:
			if (bCaps)
				wc = W_t(towupper(wc_i));
		break;

		case Language::Russian:
		switch (sym)
		{
			case SDLK_a: wc_i = bCaps ? 1060 : 1092; break;
			case SDLK_b: wc_i = bCaps ? 1048 : 1080; break;
			case SDLK_c: wc_i = bCaps ? 1057 : 1089; break;
			case SDLK_d: wc_i = bCaps ? 1042 : 1074; break;
			case SDLK_e: wc_i = bCaps ? 1059 : 1091; break;
			case SDLK_f: wc_i = bCaps ? 1040 : 1072; break;
			case SDLK_g: wc_i = bCaps ? 1055 : 1087; break;
			case SDLK_h: wc_i = bCaps ? 1056 : 1088; break;
			case SDLK_i: wc_i = bCaps ? 1064 : 1096; break;
			case SDLK_j: wc_i = bCaps ? 1054 : 1086; break;
			case SDLK_k: wc_i = bCaps ? 1051 : 1083; break;
			case SDLK_l: wc_i = bCaps ? 1044 : 1076; break;
			case SDLK_m: wc_i = bCaps ? 1068 : 1100; break;
			case SDLK_n: wc_i = bCaps ? 1058 : 1090; break;
			case SDLK_o: wc_i = bCaps ? 1065 : 1097; break;
			case SDLK_p: wc_i = bCaps ? 1047 : 1079; break;
			case SDLK_q: wc_i = bCaps ? 1049 : 1081; break;
			case SDLK_r: wc_i = bCaps ? 1050 : 1082; break;
			case SDLK_s: wc_i = bCaps ? 1067 : 1099; break;
			case SDLK_t: wc_i = bCaps ? 1045 : 1077; break;
			case SDLK_u: wc_i = bCaps ? 1043 : 1075; break;
			case SDLK_v: wc_i = bCaps ? 1052 : 1084; break;
			case SDLK_w: wc_i = bCaps ? 1062 : 1094; break;
			case SDLK_x: wc_i = bCaps ? 1063 : 1095; break;
			case SDLK_y: wc_i = bCaps ? 1053 : 1085; break;
			case SDLK_z: wc_i = bCaps ? 1071 : 1103; break;
			case SDLK_LEFTBRACKET: wc_i = bCaps ? 1061 : 1093; break;
			case SDLK_RIGHTBRACKET: wc_i = bCaps ? 1066 : 1098; break;
			case SDLK_SEMICOLON: wc_i = bCaps ? 1046 : 1078; break;
			case SDLK_QUOTE: wc_i = bCaps ? 1069 : 1101; break;
			case SDLK_COMMA: wc_i = bCaps ? 1041 : 1073; break;
			case SDLK_PERIOD: wc_i = bCaps ? 1070 : 1102; break;
			case SDLK_BACKQUOTE: wc_i = bCaps ? 1025 : 1105; break;
			default: break; //not supported
		}
		wc=W_t(wc_i);
		break;
	}
}

//
//CWidget private methods.
//

//***************************************************************************
void CWidget::ClipWHToDest()
//Clip width and height of widget area to fit inside dest surface.
{
	SDL_Surface *pDestSurface = GetDestSurface();
	if (this->x + static_cast<int>(this->w) > pDestSurface->w)
		this->w = pDestSurface->w - this->x;
	if (this->y + static_cast<int>(this->h) > pDestSurface->h)
		this->h = pDestSurface->h - this->y;
}

//***************************************************************************
void CWidget::AddHotkey(
//Adds hotkey with given key and tag to this object.
//
//Params:
	const SDL_Keycode key, //(in)
	const UINT tag)  //(in)
{
	//Is key already set as a hotkey?
	UINT wIndex;
	for (wIndex = 0; wIndex<this->hotkeys.size(); ++wIndex)
		if (this->hotkeys[wIndex].key == key)
		{
			//Change this key to new tag
			this->hotkeys[wIndex].tag = tag;
			return;
		}

	//Add new hotkey assignment.
	const HOTKEY hotkey = {key, tag};
	this->hotkeys.push_back(hotkey);
}

//***************************************************************************
UINT CWidget::GetHotkeyTag(
//Returns: tag attached to hotkey 'key' for this object or any of its children if it exists, else 0.
//
//Params:
	const SDL_Keysym& keysym) //(in)
{
	//Only check active widgets.
	if (!IsActive())
		return 0;

	UINT wTag = this->GetHotkeyTagInSelf(keysym);
	if (wTag == 0){
		wTag = this->GetHotkeyTagInChildren(keysym);
	}

	//No hotkey mappings found for this key.
	return wTag;
}

//***************************************************************************
UINT CWidget::GetHotkeyTagInSelf(
	//Returns: tag attached to hotkey 'key' for this object if it exists, else 0.
	//
	//Params:
	const SDL_Keysym& keysym) //(in)
{
	if (!IsActive())
		return 0;

	//Is key set as a hotkey?
	if (!this->hotkeys.empty())
	{
		//Consider Unicode key mappings.
		const bool bScancode = (keysym.sym & SDLK_SCANCODE_MASK) != 0;
		WCHAR wc = We(0);
		if (!bScancode) {
			wc = TranslateUnicodeKeysym(keysym, false);
			wc = towlower(wc);
		}
		UINT wIndex;
		for (wIndex = this->hotkeys.size(); wIndex--; ) {
			const HOTKEY& hotkey = this->hotkeys[wIndex];
			if (bScancode) {
				if (hotkey.key == keysym.sym)
					return hotkey.tag;
			} else if (hotkey.key == (SDL_Keycode)WCv(wc)) {
				return hotkey.tag;
			}
		}

		if (wc > 255 && !bScancode) //Unicode hotkeys might have not been successfully
		{             //converted to lower case, and still be upper case
			TranslateUnicodeKeysym(wc, keysym.sym, true);
			for (wIndex = this->hotkeys.size(); wIndex--; )
				if (this->hotkeys[wIndex].key == (SDL_Keycode)WCv(wc))
					return this->hotkeys[wIndex].tag;
		}
	}

	return 0;
}


//***************************************************************************
UINT CWidget::GetHotkeyTagInChildren(
	//Returns: tag attached to hotkey 'key' for any of the children if it exists, else 0.
	//
	//Params:
	const SDL_Keysym& keysym) //(in)
{
	if (!IsActive())
		return 0;

	//Recursively check children's hotkeys.
	for (WIDGET_ITERATOR iSeek = this->Children.begin();
		iSeek != this->Children.end(); ++iSeek)
	{
		const UINT tag = (*iSeek)->GetHotkeyTag(keysym);
		if (tag)
			return tag;
	}

	return 0;
}

//***************************************************************************
void CWidget::RemoveHotkey(
//Removes hotkey with given tag from this object.
//
//Params:
	const UINT tag)  //(in)
{
	//Find tag.
	UINT wIndex;
	for (wIndex = 0; wIndex<this->hotkeys.size(); ++wIndex)
		if (this->hotkeys[wIndex].tag == tag)
		{
			//Remove hotkey.
			//Slide further ones up in the array one spot.
			while (wIndex<this->hotkeys.size()-1)
			{
				this->hotkeys[wIndex] = this->hotkeys[wIndex+1];
				++wIndex;
			}
			//Remove last (empty) spot.
			this->hotkeys.pop_back();
			break;
		}
}

//***************************************************************************
void CWidget::RemoveAllHotkeys()
//Deallocates all hotkeys in this object.
{
	this->hotkeys.clear();
}
