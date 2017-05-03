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

#include "HyperLinkWidget.h"
#include "EventHandlerWidget.h"

#include <BackEndLib/Wchar.h>

//
//Constants.
//

const UINT NUM_WORLDLINKS = 3;
const WCHAR wszWorldLinks[NUM_WORLDLINKS][9] = {
	{We('h'),We('t'),We('t'),We('p'),We(':'),We(0)},
	{We('h'),We('t'),We('t'),We('p'),We('s'),We(':'),We(0)},
	{We('w'),We('w'),We('w'),We('.'),We(0)}
};

//
//Public methods.
//

//*****************************************************************************
CHyperLinkWidget::CHyperLinkWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY,     //       constructor.
	const UINT wSetW, const UINT wSetH,   //
	const UINT eSetFontType,              //(in)   Font to use for text.
	const UINT eSetFontType2,             //(in)   Font to use for active text.
	const WCHAR *pwczSetText,             //(in)   Text that label will display.
	const WCHAR *pwczSetLink,             //(in)   Link that this points to.
	const bool bResizeToFit,              //(in)   If true, widget height will
	                                      //       change to match height of
	                                      //       rendered text.  Default is
	                                      //       false.  See comments in
	                                      //       SetText().
	const UINT wFirstIndent,              //(in)   Indentation of the first line.
	const bool bCacheRendering)
	: CLabelWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH,
		eSetFontType, pwczSetText, bResizeToFit, wFirstIndent, WT_HyperLink, bCacheRendering)
	, eFontType1(eSetFontType), eFontType2(eSetFontType2)
{
	SetLink(pwczSetLink);
}

//*****************************************************************************
CHyperLinkWidget::~CHyperLinkWidget()
//Destructor
{
	//Remove hovering.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler)
		pEventHandler->RemoveHoveringWidget(this);
}

//*****************************************************************************
bool CHyperLinkWidget::ContainsCoords(
//Does the widget's area contain specified coordinates?
//
//Returns:
//True if it does, false if not.
//
//Params:
	const int nX, const int nY) //(in) Coords to compare against.
const
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	UINT eFontType = GetFontType();

	const int nRelX = nX - this->x - nOffsetX;
	const int nRelY = nY - this->y - nOffsetY;

	if (nRelX < 0 || (UINT)nRelX >= this->w ||
			nRelY < (int)g_pTheFM->AdjustTextRectHeight(eFontType, 0) || (UINT)nRelY >= this->h)
		return false;

	const UINT wFontH = g_pTheFM->GetFontHeight(eFontType);
	if ((UINT)nRelY < wFontH)
	{
		if (nRelY < (int)(this->h - wFontH))
			return ((UINT)nRelX >= GetFirstIndent());
		return ((UINT)nRelX >= GetFirstIndent() && (UINT)nRelX < GetLastWidth());
	}
	else
	{
		if (nRelY < (int)(this->h - wFontH))
			return true;
		return ((UINT)nRelX < GetLastWidth());
	}
}

//*****************************************************************************
void CHyperLinkWidget::HandleMouseMotion(
	const SDL_MouseMotionEvent &/*Motion*/)
{
	if (GetFontType() != this->eFontType2)
	{
		SetFontType(this->eFontType2);
		RequestPaint();
	}
}

//*****************************************************************************
void CHyperLinkWidget::HandleMouseOut()
{
	if (GetFontType() != this->eFontType1)
	{
		SetFontType(this->eFontType1);
		RequestPaint();
	}
}

//*****************************************************************************
bool CHyperLinkWidget::IsExternal()
//Returns: whether the link matches any form of possible external link.
{
	for (UINT i=0; i<NUM_WORLDLINKS; ++i)
		if (!this->wstrLink.compare(0, WCSlen(wszWorldLinks[i]), wszWorldLinks[i]))
			return true;
	return false;
}
