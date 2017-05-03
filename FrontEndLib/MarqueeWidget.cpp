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
 * Contributor(s): Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#include "MarqueeWidget.h"
#include "EventHandlerWidget.h"
#include <BackEndLib/Assert.h>

#define FXP_SHIFT 8

//***************************************************************************
CMarqueeWidget::CMarqueeWidget(const UINT dwSetTagNo,
	const int nSetX, const int nSetY,
	const UINT wSetW, const UINT wSetH, const UINT wSetSpeed)
	: CWidget(WT_Marquee, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, wSpeed(wSetSpeed << FXP_SHIFT), wTargetSpeed(wSetSpeed << FXP_SHIFT)
	, wAccelTicks(600), nFirstPos(wSetW)
{
	this->wLastTick = SDL_GetTicks();
}

//***************************************************************************
CMarqueeWidget::~CMarqueeWidget()
{
	RemoveParts();
}

//***************************************************************************
CWidget* CMarqueeWidget::GetWidgetPart(const UINT dwTagNo)
//Returns: widget contained in a part with queried tag, or NULL if not found.
{
	for (PART_ITERATOR iPart = this->Parts.begin(); iPart != this->Parts.end(); ++iPart)
	{
		if ((*iPart)->type == MP_Widget)
		{
			CWidget *pWidget = (*iPart)->GetWidget();
			if (pWidget->GetTagNo() == dwTagNo)
				return pWidget;
		}
	}

	return NULL;
}

//***************************************************************************
CWidget* CMarqueeWidget::GetWidgetContainingCoords(
//Gets widget that contains coords.  Recursively called on child widgets
//to get the widget with a smaller area containing the coords.  If two or more
//widget siblings contain the coords, then the first one found will be returned.
//Hidden and disabled widgets are ignored, as well as children clicked outside
//the parent's clipping region.
//
//Params:
	const int nX, const int nY,    //(in)  Coords to check against.
	const WIDGETTYPE eWidgetType)  //(in)  Type of widget required for a match.
											 //      If WT_Unspecified, criteria will not
											 //      be applied.
//
//Returns:
//Found widget or NULL if no match.
const
{
	int pX = this->x + (this->nFirstPos >> FXP_SHIFT);
	for (ACTIVEPART_ITERATOR iActive = this->ActiveParts.begin();
			iActive != this->ActiveParts.end(); ++iActive)
	{
		if (nX >= pX && nX < pX + (int)((*iActive)->GetW()))
		{
			if (!(*iActive)->IsEnabled() || (*iActive)->IsRemoved())
				break;

			CWidget *pWidget = (*iActive)->GetWidget();
			pWidget->Move(pX - this->x, 0);
			if (pWidget->ContainsCoords(nX, nY)) {
				CWidget *pSub = pWidget->GetWidgetContainingCoords(nX, nY, eWidgetType);
				if (pSub && (eWidgetType == WT_Unspecified || eWidgetType == pSub->GetType()))
					return pSub;
				if (eWidgetType == WT_Unspecified || eWidgetType == pWidget->GetType())
					return pWidget;
				return NULL;
			}
		}
		pX += (*iActive)->GetW();
	}
	return NULL;
}

//***************************************************************************
void CMarqueeWidget::HandleAnimate()
//Update the position of the visible children (and Paint()).  This allows
//for widget multiplexing.  Position based events are also taken care of.
//
//Params:
{
	UINT wTick = SDL_GetTicks();
	UINT wTickDiff = wTick - this->wSourceSpeedTick;

	if (this->wSpeed != this->wTargetSpeed) {
		if (wTickDiff < this->wAccelTicks) {
			int nSpeedDiff = (int)(this->wTargetSpeed - this->wSourceSpeed);

			this->wSpeed = this->wSourceSpeed +
					((nSpeedDiff * (int)wTickDiff) / (int)this->wAccelTicks);

			if (nSpeedDiff >= 0
					? (int)this->wSpeed > (int)this->wTargetSpeed
					: (int)this->wSpeed < (int)this->wTargetSpeed)
				this->wSpeed = this->wTargetSpeed;
		} else
			this->wSpeed = this->wTargetSpeed;
	}

	wTickDiff = wTick - this->wLastTick;
	this->wLastTick = wTick;

	if (wTickDiff > 1000 || !this->Parts.size()) //Autopause if inactive for >1 second
	{
		Paint(true);
		return;
	}

	//Get first visible child and its position
	const UINT wScrollAmount = wTickDiff * (this->wSpeed >> FXP_SHIFT);
	PART_ITERATOR iPart = this->iFirstPart;
	int nAdd, nEnd = this->nFirstPos - wScrollAmount;

	//Remove parts that have left the screen
	if (iPart != this->Parts.end())
	{
		while (nEnd + (nAdd = (int)((*iPart)->GetW() << FXP_SHIFT)) < 0)
		{
			if (this->ActiveParts.size()) {
				CMarqueePart *pPart = this->ActiveParts.front();
				this->ActiveParts.pop_front();
				pPart->DecUse();
			}

			nEnd += nAdd;
			if (++iPart == this->Parts.end())
				iPart = this->Parts.begin();
		}
	}
	this->nFirstPos = nEnd;
	this->iFirstPart = iPart;

	UINT wW = this->w << FXP_SHIFT;

	//Count parts that are still on the screen
	for (ACTIVEPART_ITERATOR iActive = this->ActiveParts.begin();
			iActive != this->ActiveParts.end()
			&& (UINT)(nEnd += (int)((*iActive)->GetW() << FXP_SHIFT)) <= wW; )
	{
		if ((*iPart)->IsEnabled() && !(*iPart)->IsRemoved())
			++iActive;
		if (++iPart == this->Parts.end())
			iPart = this->Parts.begin();
	}

	//Add parts that just entered the screen
	int nLastEnd = nEnd;
	while ((UINT)nEnd <= wW && this->Parts.size())
	{
		CMarqueePart *pPart = (*iPart);
		if (pPart->IsRemoved()) {
			RemovePart(iPart);
			if (this->Parts.empty())
				break;	//no more parts to show
		} else if (pPart->IsEnabled()) {
			pPart->IncUse();
			this->ActiveParts.push_back(pPart);
			switch (pPart->GetType()) {
				case MP_Widget:
					nEnd += (pPart->GetW() << FXP_SHIFT);
					break;
				case MP_Speed:
					this->wSourceSpeedTick = this->wLastTick;
					this->wSourceSpeed = this->wSpeed;
					this->wTargetSpeed = pPart->GetWArg() << FXP_SHIFT;
					break;
				case MP_AccelTicks:
					this->wAccelTicks = pPart->GetWArg();
				default:
					break;
			}
			++iPart;
		}
		//Show (space-filling) parts multiple times to fill up the width of the marquee.
		if (iPart == this->Parts.end())
		{
			if (nEnd > nLastEnd)
			{
				iPart = this->Parts.begin();
				nLastEnd = nEnd;
			} else {
				//There are no parts in the marquee that take up space.
				break;
			}
		}
	}

	//Fix position-based events
	GetEventHandlerWidget()->UpdateMotion();

	//Paint it
	Paint(true);
}

//***************************************************************************
void CMarqueeWidget::Paint(
//Paint the marquee.
//
//Params:
	bool bUpdateRect)   //(in) Update screen ?
{
	//Update position and paint each visible child.
	int nPos = this->nFirstPos;

	for (ACTIVEPART_ITERATOR iActive = this->ActiveParts.begin();
			iActive != this->ActiveParts.end(); ++iActive)
	{
		if ((*iActive)->GetType() == MP_Widget) {
			CWidget *pWidget = (*iActive)->GetWidget();
			pWidget->Move(nPos >> FXP_SHIFT, 0);
			pWidget->PaintClipped(this->x, this->y, this->w, this->h, false);
			nPos += pWidget->GetW() << FXP_SHIFT;
		}
	}

	if (bUpdateRect) UpdateRect();
}

//***************************************************************************
void CMarqueeWidget::Reset()
//Reset the scroller.  Also used for initing.  FIXME
{
	while (this->ActiveParts.size()) {
		CMarqueePart *pPart = this->ActiveParts.front();
		this->ActiveParts.pop_front();
		pPart->DecUse();
	}

	this->iFirstPart = this->Parts.begin();
	this->nFirstPos = this->w << FXP_SHIFT;
	this->wLastTick = SDL_GetTicks();
}

//***************************************************************************
CMarqueePart* CMarqueeWidget::AddPart(
//Add a widget-part to the scroller.
	CWidget *pWidget)   //(in)   Part to be added.
//
//Returns:
//The part that was added.
{
	CMarqueePart *pPart = new CMarqueePart(this, pWidget);
	this->Parts.push_back(pPart);
	return pPart;
}

//***************************************************************************
CMarqueePart* CMarqueeWidget::AddPart(
//Add a control-part to the scroller.
	MP_Type type,   //(in)   Part type.
	UINT    arg)    //(in)   Type argument.
//
//Returns:
//The part that was added.
{
	CMarqueePart *pPart = new CMarqueePart(this, type, arg);
	this->Parts.push_back(pPart);
	return pPart;
}

//***************************************************************************
CMarqueePart* CMarqueeWidget::AddPart(
//Add a part to the scroller.
	CMarqueePart *pPart)//(in)   Part.
//
//Returns:
//The part that was added.
{
	this->Parts.push_back(pPart);
	return pPart;
}

//***************************************************************************
void CMarqueeWidget::RemoveParts()
//Removes all parts from the marquee.
{
   Reset();
   for (PART_ITERATOR iPart = this->Parts.begin(); iPart != this->Parts.end(); ++iPart)
		delete *iPart;
   this->Parts.clear();
   this->iFirstPart = this->Parts.begin();
}

//
// Protected methods
//

//***************************************************************************
void CMarqueeWidget::RemovePart(
//Remove a part from the scroller and delete it, if it's unused.
//Don't call this directly, call the part's Remove method instead.
	PART_ITERATOR& iPart)   //(in/out) iterator to part to remove
{
	if ((*iPart)->GetUseCount() > 0)
		return;
	delete *iPart;
	iPart = this->Parts.erase(iPart);
}
