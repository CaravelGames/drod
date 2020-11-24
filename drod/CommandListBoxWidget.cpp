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

#include "CommandListBoxWidget.h"
#include "DrodFontManager.h"

//
//Public methods.
//

//******************************************************************************
CCommandListBoxWidget::CCommandListBoxWidget(
	//Constructor.
	//
	//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH)
	: CListBoxWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH, false, true, true)
{}

UINT CountIndent(const WSTRING &wstr) {
	UINT count = 0;
	for (UINT i = 0; i < wstr.size(); ++i)
	{
		if (wstr.at(i) == wszSpace[0])
			count++;
		else
			break;
	}

	return count;
}

//******************************************************************************
void CCommandListBoxWidget::Paint_Line(const UINT wListItemNumber, const UINT wDrawLineNumber, const LBOX_ITEM &listItem)
{
	CListBoxWidget::Paint_Line(wListItemNumber, wDrawLineNumber, listItem);

	int drawX, drawY;
	UINT drawWidth, drawHeight;
	GetLineDrawCoords(wDrawLineNumber, drawX, drawY, drawWidth, drawHeight);

	const UINT indent = CountIndent(listItem.text);
	static const UINT LINE_LEFT_PADDING = 5;
	static const UINT INDENT_WIDTH = 3;
	static const UINT SPACE_WIDTH = g_pTheDFM->GetSpaceWidth(FONTLIB::F_ListBoxItem);
	static const SURFACECOLOR LINE_COLOR = { 170, 170, 170 };

	SDL_Surface *pDestSurface = GetDestSurface();

	for (UINT i = 8; i < indent; i += 3)
	{
		// Hack to avoid drawing line in the if's condition which is indented like this:
		//        If ...                         <- Default indent is 8
		//             Wait for clean room       <- If conditions have indent of 5
		//           Speech "",Normal,Self,0,.   <- If body has indent of 3
		//        If End
		if (i + 5 == indent)
			break;

		DrawCol(drawX + i * SPACE_WIDTH + LINE_LEFT_PADDING, drawY, drawHeight, LINE_COLOR, pDestSurface);
	}

	{ // Draw line number
		static const UINT LINE_NUMBER_FONT = FONTLIB::F_Small;
		static const UINT LINE_NUMBER_WIDTH = 35;
		static const SDL_Color LINE_NUMBER_COLOR = { 120, 120, 120, 0 };

		WCHAR buffer[12];
		WSTRING wstrLineNumber;
		wstrLineNumber += _itoW(wListItemNumber, buffer, 10, 12);

		SDL_Color oldColor = g_pTheFM->GetFontColor(LINE_NUMBER_FONT);
		UINT wTextWidth;
		g_pTheFM->SetFontColor(LINE_NUMBER_FONT, LINE_NUMBER_COLOR);
		g_pTheFM->GetTextWidth(LINE_NUMBER_FONT, wstrLineNumber.c_str(), wTextWidth);
		g_pTheFM->DrawTextXY(
			LINE_NUMBER_FONT, wstrLineNumber.c_str(), pDestSurface,
			drawX + LINE_NUMBER_WIDTH - wTextWidth, drawY - 5,
			wTextWidth, CY_LBOX_ITEM
		);
		g_pTheFM->SetFontColor(LINE_NUMBER_FONT, oldColor);
	}
}
