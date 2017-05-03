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

#include "LabelWidget.h"
#include "BitmapManager.h"

#include <BackEndLib/Wchar.h>

//
//Public methods.
//

//*****************************************************************************
CLabelWidget::CLabelWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY,               //    constructor.
	const UINT wSetW, const UINT wSetH,             //
	const UINT eSetFontType,            //(in)   Font to use for text.
	const WCHAR *pwczSetText,           //(in)   Text that label will display.
	const bool bResizeToFit,                  //(in)   If true, widget height will
											//    change to match height of
											//    rendered text.  Default is
											//    false.  See comments in
											//    SetText().
	const UINT wFirstIndent,            //(in)   Indentation of the first line [default=0]
	const WIDGETTYPE eType,       //[default=WT_Label]
	const bool bCacheRendering,   //[default=false]
	const int y_font_offset)
	: CWidget(eType, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, eFontType(eSetFontType)
	, eTextAlign(TA_Left)
	, bClickable(false)
	, bCacheRendering(bCacheRendering)
	, pRenderedText(NULL)
	, y_font_offset(y_font_offset)
{
	SetText(pwczSetText, bResizeToFit, wFirstIndent);
}

CLabelWidget::~CLabelWidget()
{
	if (this->pRenderedText)
		SDL_FreeSurface(this->pRenderedText);
}

//*****************************************************************************
bool CLabelWidget::ContainsCoords(const int nX, const int nY) const
//Returns: whether widget contains coords only if widget is specified as clickable
{
	if (!this->bClickable)
		return false;

	return CWidget::ContainsCoords(nX, nY);
}

//*****************************************************************************
UINT CLabelWidget::GetTextWidthHeight(
//Gets width and height of text as it is drawn within label.
//
//Params:
	UINT &wW, UINT &wH,  //(out) Width and height of text.
	const UINT wFirstIndent)   //(in)  First line pixel indent (optional).
//
//Returns:
//The width of the last line.
const
{
	if (this->wstrText.size()==0)
	{
		this->wLastWidth = wW = wH = 0;
		return 0;
	}

	//Ask font manager about it.
	return this->wLastWidth = g_pTheFM->GetTextRectHeight(this->eFontType,
			this->wstrText.c_str(), this->w, wW, wH, wFirstIndent);
}

//*****************************************************************************
void CLabelWidget::SetFontType(const UINT eSetFontType) {
	this->eFontType = eSetFontType;

	if (this->bCacheRendering) {
		//Rerender text in new font.
		const WSTRING temp = this->wstrText;
		SetText(temp.c_str());
	}
}

//*****************************************************************************
void CLabelWidget::SetText(
//Sets text of widget.
//
//Params:
	const WCHAR *pwczSetText,  //(in)   New text.
	const bool bResizeToFit,   //(in)   If true, then height will be increased
								//    or reduced to fit the text exactly.
								//    Default is false.  If your label text
								//    is constant, it is better for performance
								//    to not resize to fit.
	const UINT wFirstIndent,   //(in)   Indentation of the first line.
	const bool bFitWidth)	//(in) If true, widget width will be resized to fit
								//the text exactly. [default=false]
{
	this->wstrText = (pwczSetText != NULL) ? pwczSetText : wszEmpty;
	this->wFirstIndent = wFirstIndent;
	this->wLastWidth = 0;

	if (bFitWidth)
	{
		//Get text height from the font manager.
		UINT wTextW, wTextH;
		g_pTheFM->GetTextWidthHeight(this->eFontType, this->wstrText.c_str(),
				wTextW, wTextH);

		//Resize width to fit text.
		SetWidth(wTextW + wFirstIndent);
	}
 
 	if (bResizeToFit)
	{
		//Get text height from the font manager.
		UINT wTextW, wTextH;
		this->wLastWidth = g_pTheFM->GetTextRectHeight(this->eFontType,
				this->wstrText.c_str(), this->w, wTextW, wTextH, wFirstIndent);

		//Resize height to fit text.
		SetHeight(g_pTheFM->AdjustTextRectHeight(this->eFontType, wTextH));
	}

	if (this->bCacheRendering)
		RenderAndCacheText();
}

//*****************************************************************************
void CLabelWidget::Paint(
//Paint text inside of the widget area.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	if (this->wstrText.size())
	{
		int nOffsetX, nOffsetY;
		GetScrollOffset(nOffsetX, nOffsetY);

		UINT wLineOffsetX = 0;

		//Get drawing X coord for centered text.
		if (this->eTextAlign == TA_CenterGroup)
		{
			UINT wLongestLineW, wH;
			g_pTheFM->GetTextRectHeight(this->eFontType, this->wstrText.c_str(),
					this->w, wLongestLineW, wH);
			wLineOffsetX = (this->w - wLongestLineW) / 2;
		}

		if (this->pRenderedText) {
			SDL_Rect rect = MAKE_SDL_RECT(this->x + wLineOffsetX + nOffsetX, this->y + nOffsetY, this->w, this->h);
			SDL_BlitSurface(this->pRenderedText, NULL, GetDestSurface(), &rect);
		} else {
			g_pTheFM->DrawTextToRect(this->eFontType, this->wstrText.c_str(),
					this->x + wLineOffsetX + nOffsetX, this->y + nOffsetY,
					this->w, this->h, GetDestSurface(), this->wFirstIndent);
		}
	}

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
void CLabelWidget::RenderAndCacheText()
// Create cached bitmap supporting fancy font rendering with alpha.
{
	if (this->pRenderedText) {
		SDL_FreeSurface(this->pRenderedText);
		this->pRenderedText = NULL;
	}

	//Draw the text.
	if (g_pTheFM->GetFontAntiAlias(this->eFontType) && g_pTheFM->GetFontOutline(this->eFontType) &&
			!this->wstrText.empty())
	{
		this->pRenderedText = CBitmapManager::ConvertSurface(
			SDL_CreateRGBSurface(SDL_SWSURFACE, this->w, this->h - y_font_offset, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
		const Uint32 color = SDL_MapRGBA(this->pRenderedText->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_FillRect(this->pRenderedText, NULL, color);

		g_pTheFM->DrawTextToRect(this->eFontType, this->wstrText.c_str(),
				0, this->y_font_offset, this->w, this->h, this->pRenderedText, this->wFirstIndent);
	}
}
