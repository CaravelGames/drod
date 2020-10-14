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
 *
 * ***** END LICENSE BLOCK ***** */

#include "FocusWidget.h"
#include "EventHandlerWidget.h"

//
//Public methods.
//

//******************************************************************************
CFocusWidget::CFocusWidget(
//Constructor.
//
//Params:
	WIDGETTYPE eSetType,          //(in)   Required params for CWidget constructor.
	UINT dwSetTagNo,                //
	int nSetX, int nSetY,            //
	UINT wSetW, UINT wSetH)       //
	: CWidget(eSetType, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bSelected(false)
	, bFocusingAllowed(true)
{
}

//******************************************************************************
void CFocusWidget::Select(
//Selects the widget.
//Repaints if 'bPaint' is set.
//
//Params:
	const bool bPaint)   //(in) whether to repaint widget
{
	ASSERT(IsActive());
	this->bSelected = true;
	if (bPaint)
		RequestPaint();
}

//******************************************************************************
void CFocusWidget::Unselect(
//Unselects the widget.
//Repaints if visible and 'bPaint' is set.
//
//Params:
	const bool bPaint)   //(in) whether to repaint widget
{
	this->bSelected = false;
	if (bPaint && IsVisible(true))
		RequestPaint();
}

//******************************************************************************
void CFocusWidget::Disable()
//CWidget override that updates selection.
{
	CWidget::Disable();
	if (this->bSelected)
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->SelectNextWidget();
	}
}

//******************************************************************************
void CFocusWidget::Hide(
//CWidget override that updates selection.
//
//Params:
	const bool bPaint)   //(in) whether to repaint next widget selected
{
	CWidget::Hide();
	if (this->bSelected)
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->SelectNextWidget(bPaint);
	}
}

//******************************************************************************
void CFocusWidget::SetFocusAllowed(const bool bVal) //[default=true]
//Determines whether this widget may receive focus when added to another widget as a child.
//
//Pre-condition: this widget doesn't already belong to a widget (i.e. this must be set before adding this widget)
{
	ASSERT(!this->pParent);
	this->bFocusingAllowed = bVal;
}
